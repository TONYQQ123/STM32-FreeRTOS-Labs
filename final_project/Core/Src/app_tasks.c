/**
 * @file    app_tasks.c
 * @brief   Pub-Sub v3：DHT11 + 傾斜感測器 → OLED + LED + Alarm
 *
 * Topic 架構：
 *
 *  sensor_task ─► "temperature"  ─► ui_task (OLED 第0行)
 *              ─► "humidity"     ─► ui_task (OLED 第1行)
 *              ─► "tilt"         ─► ui_task (OLED 第3行)
 *                                ─► control_task (LED + 發 alarm)
 *                                       │
 *                                       ▼ "alarm"
 *                                  alarm_task (LED 閃 + UART)
 */

#include "app_tasks.h"
#include "pubsub.h"
#include "uart_log.h"
#include "dht11.h"
#include "ssd1315.h"
#include "main.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* -----------------------------------------------------------------------
 * 外部 HAL handle（main.c 定義）
 * ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
 * Task 設定
 * ----------------------------------------------------------------------- */
#define TASK_STACK_SIZE      320u   /* 多一點，snprintf 需要空間 */
#define SENSOR_TASK_PRI      3u
#define SUB_TASK_PRI         2u
#define SENSOR_INTERVAL_MS   2000u  /* DHT11 最快 1 次/2 秒 */

/* 傾斜感測器：PC1，低電位 = 傾斜（大多數模組 DO 平時 HIGH，傾斜 LOW） */
#define TILT_PORT   GPIOC
#define TILT_PIN    GPIO_PIN_1

/* LED */
#define LED_PORT        GPIOD
#define LED_GREEN_PIN   GPIO_PIN_12
#define LED_ORANGE_PIN  GPIO_PIN_13
#define LED_RED_PIN     GPIO_PIN_14
#define LED_BLUE_PIN    GPIO_PIN_15

/* 溫度門檻 */
#define TEMP_WARN   28
#define TEMP_HOT    33
#define TEMP_ALARM  36

/* -----------------------------------------------------------------------
 * LED helpers
 * ----------------------------------------------------------------------- */
static void led_all_off(void)
{
    HAL_GPIO_WritePin(LED_PORT,
        LED_GREEN_PIN | LED_ORANGE_PIN | LED_RED_PIN | LED_BLUE_PIN,
        GPIO_PIN_RESET);
}

static void led_set(uint16_t pins)
{
    led_all_off();
    HAL_GPIO_WritePin(LED_PORT, pins, GPIO_PIN_SET);
}

/* -----------------------------------------------------------------------
 * Subscriber instances（必須 static）
 * ----------------------------------------------------------------------- */
static Subscriber sub_ui       = { .name = "UI"      };
static Subscriber sub_control  = { .name = "Control" };
static Subscriber sub_alarm    = { .name = "Alarm"   };

/* -----------------------------------------------------------------------
 * Sensor Task：每 2 秒讀一次所有感測器，各自 publish
 * ----------------------------------------------------------------------- */
static void sensor_task(void *arg)
{
    (void)arg;
    DHT11_Data  dht  = {0};
    char        payload[MAX_PAYLOAD_LENGTH];

    /* 等所有 subscriber 準備好 */
    vTaskDelay(pdMS_TO_TICKS(500));

    for (;;) {
        /* ---- DHT11 ---- */
        if (DHT11_Read(&dht) == DHT11_OK) {
            snprintf(payload, sizeof(payload), "%d", dht.temperature);
            publish(TOPIC_TEMPERATURE, payload);

            snprintf(payload, sizeof(payload), "%d", dht.humidity);
            publish(TOPIC_HUMIDITY, payload);
        } else {
            uart_logf("[Sensor] DHT11 read error\r\n");
        }

        /* ---- 傾斜感測器（數位 GPIO） ---- */
        /* 大多數模組：平放 HIGH，傾斜 LOW */
        GPIO_PinState tilt_state = HAL_GPIO_ReadPin(TILT_PORT, TILT_PIN);
        publish(TOPIC_TILT, tilt_state == GPIO_PIN_RESET ? "1" : "0");

        vTaskDelay(pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
    }
}

/* -----------------------------------------------------------------------
 * UI Task：收到任何 topic → 更新 OLED 對應行
 *
 * OLED 版面：
 *   Row 0: Temp: 25 C
 *   Row 1: Humi: 60 %
 *   Row 2: Lux : 312
 *   Row 3: Tilt: NO   (或 YES!)
 * ----------------------------------------------------------------------- */
static void ui_task(void *arg)
{
    (void)arg;
    Message msg;

    /* OLED 初始化 */
    SSD1315_Init(&hi2c1);
    SSD1315_Clear(&hi2c1);
    SSD1315_WriteString(&hi2c1, 0, 0, "=== PubSub v3 ===");
    vTaskDelay(pdMS_TO_TICKS(1500));
    SSD1315_Clear(&hi2c1);

    /* 預填標籤 */
    SSD1315_WriteString(&hi2c1, 0, 0, "Temp:     C");
    SSD1315_WriteString(&hi2c1, 0, 1, "Humi:     %");
    SSD1315_WriteString(&hi2c1, 0, 2, "Lux :     ");
    SSD1315_WriteString(&hi2c1, 0, 3, "Tilt: ---");

    char line[17];

    for (;;) {
        if (xQueueReceive(sub_ui.queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        if (strcmp(msg.topic, TOPIC_TEMPERATURE) == 0) {
            snprintf(line, sizeof(line), "Temp: %3s C", msg.payload);
            SSD1315_WriteString(&hi2c1, 0, 0, line);

        } else if (strcmp(msg.topic, TOPIC_HUMIDITY) == 0) {
            snprintf(line, sizeof(line), "Humi: %3s %%", msg.payload);
            SSD1315_WriteString(&hi2c1, 0, 1, line);

        } else if (strcmp(msg.topic, TOPIC_TILT) == 0) {
            if (msg.payload[0] == '1') {
                SSD1315_WriteString(&hi2c1, 0, 3, "Tilt: YES !");
            } else {
                SSD1315_WriteString(&hi2c1, 0, 3, "Tilt: no  ");
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * Control Task：收溫度 → LED，超門檻或傾斜 → publish alarm
 * ----------------------------------------------------------------------- */
static void control_task(void *arg)
{
    (void)arg;
    Message msg;
    int     last_alarm = 0;

    for (;;) {
        if (xQueueReceive(sub_control.queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        if (strcmp(msg.topic, TOPIC_TEMPERATURE) == 0) {
            int t = atoi(msg.payload);

            /* LED 狀態 */
            if      (t >= TEMP_ALARM) led_set(LED_RED_PIN | LED_BLUE_PIN);
            else if (t >= TEMP_HOT)   led_set(LED_RED_PIN);
            else if (t >= TEMP_WARN)  led_set(LED_ORANGE_PIN);
            else                      led_set(LED_GREEN_PIN);

            /* Alarm 觸發 */
            if (t >= TEMP_ALARM && !last_alarm) {
                publish(TOPIC_ALARM, "HIGH:TEMP");
                last_alarm = 1;
            } else if (t < TEMP_ALARM && last_alarm) {
                publish(TOPIC_ALARM, "CLEAR");
                last_alarm = 0;
            }

        } else if (strcmp(msg.topic, TOPIC_TILT) == 0) {
            if (msg.payload[0] == '1' && !last_alarm) {
                publish(TOPIC_ALARM, "HIGH:TILT");
                last_alarm = 1;
            } else if (msg.payload[0] == '0' && last_alarm) {
                publish(TOPIC_ALARM, "CLEAR");
                last_alarm = 0;
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * Alarm Task：收到 alarm → LED 閃爍 + UART 警告
 * ----------------------------------------------------------------------- */
static void alarm_task(void *arg)
{
    (void)arg;
    Message msg;

    for (;;) {
        if (xQueueReceive(sub_alarm.queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        if (strncmp(msg.payload, "HIGH", 4) == 0) {
            uart_logf("[ALARM] !! %s !!\r\n", msg.payload);
            for (int i = 0; i < 3; i++) {
                HAL_GPIO_WritePin(LED_PORT,
                    LED_RED_PIN | LED_BLUE_PIN, GPIO_PIN_SET);
                vTaskDelay(pdMS_TO_TICKS(200));
                led_all_off();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        } else if (strncmp(msg.payload, "CLEAR", 5) == 0) {
            uart_logf("[ALARM] Cleared\r\n");
        }
    }
}

/* -----------------------------------------------------------------------
 * Stats Task：每 30 秒印統計
 * ----------------------------------------------------------------------- */
static void stats_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        pubsub_print_stats();
    }
}

/* -----------------------------------------------------------------------
 * start_app_tasks()
 * ----------------------------------------------------------------------- */
void start_app_tasks(void)
{
    ps_status_t ret;

    /* ---- 1. 建立 queues ---- */
    sub_ui.queue      = xQueueCreate(SUBSCRIBER_QUEUE_LEN, sizeof(Message));
    sub_control.queue = xQueueCreate(SUBSCRIBER_QUEUE_LEN, sizeof(Message));
    sub_alarm.queue   = xQueueCreate(SUBSCRIBER_QUEUE_LEN, sizeof(Message));
    configASSERT(sub_ui.queue && sub_control.queue && sub_alarm.queue);

    /* ---- 2. Subscribe ---- */
    /* UI 訂閱全部 topic */
    ret = subscribe(TOPIC_TEMPERATURE, &sub_ui);     configASSERT(ret == PS_OK);
    ret = subscribe(TOPIC_HUMIDITY,    &sub_ui);     configASSERT(ret == PS_OK);
    ret = subscribe(TOPIC_TILT,        &sub_ui);     configASSERT(ret == PS_OK);

    /* Control 只訂溫度和傾斜 */
    ret = subscribe(TOPIC_TEMPERATURE, &sub_control); configASSERT(ret == PS_OK);
    ret = subscribe(TOPIC_TILT,        &sub_control); configASSERT(ret == PS_OK);

    /* Alarm task 訂 alarm topic */
    ret = subscribe(TOPIC_ALARM,       &sub_alarm);  configASSERT(ret == PS_OK);

    /* ---- 3. 建立 tasks ---- */
    BaseType_t ok;
    ok = xTaskCreate(sensor_task,  "Sensor",  TASK_STACK_SIZE, NULL, SENSOR_TASK_PRI, NULL); configASSERT(ok == pdPASS);
    ok = xTaskCreate(ui_task,      "UI",      TASK_STACK_SIZE, NULL, SUB_TASK_PRI,    NULL); configASSERT(ok == pdPASS);
    ok = xTaskCreate(control_task, "Control", TASK_STACK_SIZE, NULL, SUB_TASK_PRI,    NULL); configASSERT(ok == pdPASS);
    ok = xTaskCreate(alarm_task,   "Alarm",   TASK_STACK_SIZE, NULL, SUB_TASK_PRI,    NULL); configASSERT(ok == pdPASS);
    ok = xTaskCreate(stats_task,   "Stats",   256,             NULL, 1u,              NULL); configASSERT(ok == pdPASS);
}
