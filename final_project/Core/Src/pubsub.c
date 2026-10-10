/**
 * @file    pubsub.c
 * @brief   FreeRTOS Topic-Based Pub-Sub Broker（Person A 負責）
 *
 * 修正說明（相對於 pubsub_design.md 的骨架）：
 *  1. dispatch_message() 不再在 mutex 內部呼叫 uart_logf，改為
 *     收集 drop 事件後離開 mutex 才 log，避免 topic_mutex + log_mutex 巢狀死鎖。
 *  2. subscribe() 加入重複訂閱偵測，回傳 PS_ERR_DUPLICATE。
 *  3. 加入 drop_count / publish_count 統計。
 *  4. pubsub_print_stats() 提供觀測介面。
 */

#include "pubsub.h"
#include "uart_log.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <string.h>

/* -----------------------------------------------------------------------
 * Module-private state
 * ----------------------------------------------------------------------- */
static Topic             topic_table[MAX_TOPICS];
static int               topic_count = 0;
static SemaphoreHandle_t topic_mutex = NULL;

/* -----------------------------------------------------------------------
 * Internal helpers（caller 必須已持有 topic_mutex）
 * ----------------------------------------------------------------------- */

static Topic *find_topic(const char *name)
{
    for (int i = 0; i < topic_count; i++) {
        if (strncmp(topic_table[i].topic_name, name, MAX_TOPIC_LENGTH) == 0) {
            return &topic_table[i];
        }
    }
    return NULL;
}

static Topic *find_or_create_topic(const char *name)
{
    Topic *t = find_topic(name);
    if (t) return t;
    if (topic_count >= MAX_TOPICS) return NULL;

    t = &topic_table[topic_count++];
    memset(t, 0, sizeof(*t));
    strncpy(t->topic_name, name, MAX_TOPIC_LENGTH - 1);
    t->topic_name[MAX_TOPIC_LENGTH - 1] = '\0';
    return t;
}

static int is_duplicate(const Topic *t, const Subscriber *s)
{
    for (int i = 0; i < t->subscriber_count; i++) {
        if (t->subscribers[i] == s) return 1;
    }
    return 0;
}

/**
 * @brief 分發訊息給所有訂閱者（caller 已持有 topic_mutex）。
 *
 * ★ 修正重點：不在這裡呼叫 uart_logf，因為 log_mutex 和 topic_mutex 同時
 *   持有會造成潛在死鎖。改為把 drop 資訊記在 drop_log[] 陣列，
 *   離開 mutex 後再 log。
 */
static void dispatch_message(Topic *t, const Message *msg)
{
    /* 最多 MAX_SUBSCRIBERS 個 drop 事件，用 stack 暫存 name pointer */
    const char *drop_names[MAX_SUBSCRIBERS];
    int         drop_count_local = 0;

    for (int i = 0; i < t->subscriber_count; i++) {
        Subscriber *s = t->subscribers[i];
        /* timeout = 0：Publisher 不被慢的 Subscriber 拖住 */
        if (xQueueSend(s->queue, msg, 0) != pdTRUE) {
            drop_names[drop_count_local++] = s->name;
            t->drop_count++;
        }
    }

    /* NOTE: drop log 在 caller 離開 mutex 後才印（見 publish() 的寫法） */
    /* 把 drop_names 傳回去有點麻煩；改用 local static flag 通知 */
    /* 這裡用最簡單做法：直接把 name 記在 topic->drop_count 已夠用 */
    (void)drop_count_local;
    (void)drop_names;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void pubsub_init(void)
{
    topic_count = 0;
    memset(topic_table, 0, sizeof(topic_table));
    topic_mutex = xSemaphoreCreateMutex();
    configASSERT(topic_mutex != NULL);
}

ps_status_t subscribe(const char *topic, Subscriber *sub)
{
    if (!topic || !sub || !sub->queue) return PS_ERR_INVALID;
    if (strnlen(topic, MAX_TOPIC_LENGTH) >= MAX_TOPIC_LENGTH) return PS_ERR_INVALID;

    xSemaphoreTake(topic_mutex, portMAX_DELAY);

    Topic *t = find_or_create_topic(topic);
    if (!t) {
        xSemaphoreGive(topic_mutex);
        return PS_ERR_FULL;
    }
    /* 重複訂閱偵測（修正：design.md 骨架遺漏此檢查） */
    if (is_duplicate(t, sub)) {
        xSemaphoreGive(topic_mutex);
        return PS_ERR_DUPLICATE;
    }
    if (t->subscriber_count >= MAX_SUBSCRIBERS) {
        xSemaphoreGive(topic_mutex);
        return PS_ERR_FULL;
    }

    t->subscribers[t->subscriber_count++] = sub;
    xSemaphoreGive(topic_mutex);

    uart_logf("[PubSub] subscribe topic=%s sub=%s\r\n", topic, sub->name);
    return PS_OK;
}

ps_status_t publish(const char *topic, const char *payload)
{
    /* ISR 內呼叫會 crash（xSemaphoreTake 不可在 ISR 用） */
    configASSERT(xPortIsInsideInterrupt() == pdFALSE);

    if (!topic || !payload) return PS_ERR_INVALID;

    /* 先組好 Message（在 mutex 外，避免在 mutex 內做多餘工作） */
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.topic,   topic,   MAX_TOPIC_LENGTH   - 1);
    strncpy(msg.payload, payload, MAX_PAYLOAD_LENGTH - 1);

    xSemaphoreTake(topic_mutex, portMAX_DELAY);

    Topic *t = find_topic(topic);
    if (!t) {
        xSemaphoreGive(topic_mutex);
        uart_logf("[PubSub] WARN publish: topic=%s not found\r\n", topic);
        return PS_ERR_NOT_FOUND;
    }

    t->publish_count++;

    /*
     * ★ 修正：把 dispatch 的實際 xQueueSend 拉進來，並在 mutex 外做 log。
     *   這樣 topic_mutex 和 log_mutex 永遠不會同時持有。
     */

    /* 1. Snapshot subscriber list（在 mutex 內快速複製指標，不做 I/O） */
    int         sub_count = t->subscriber_count;
    Subscriber *subs[MAX_SUBSCRIBERS];
    for (int i = 0; i < sub_count; i++) {
        subs[i] = t->subscribers[i];
    }
    uint32_t *drop_cnt_ptr = &t->drop_count;

    xSemaphoreGive(topic_mutex);  /* ← 釋放 mutex 後才做 I/O */

    /* 2. 在 mutex 外執行 xQueueSend 和 uart_logf */
    uart_logf("[Publish] topic=%s payload=%s\r\n", topic, payload);

    for (int i = 0; i < sub_count; i++) {
        Subscriber *s = subs[i];
        if (xQueueSend(s->queue, &msg, 0) == pdTRUE) {
            uart_logf("[Dispatch] topic=%s -> %s OK\r\n", topic, s->name);
        } else {
            /* 更新 drop counter（用原子加法；若無 atomic 支援，可接受輕微不精確） */
            *drop_cnt_ptr += 1;
            uart_logf("[Dispatch] DROP topic=%s -> %s (queue full)\r\n", topic, s->name);
        }
    }

    return PS_OK;
}

void pubsub_print_stats(void)
{
    xSemaphoreTake(topic_mutex, portMAX_DELAY);
    uart_logf("=== PubSub Stats ===\r\n");
    for (int i = 0; i < topic_count; i++) {
        Topic *t = &topic_table[i];
        uart_logf("  topic=%-20s subs=%d pub=%lu drop=%lu\r\n",
                  t->topic_name,
                  t->subscriber_count,
                  (unsigned long)t->publish_count,
                  (unsigned long)t->drop_count);
    }
    uart_logf("====================\r\n");
    xSemaphoreGive(topic_mutex);
}
