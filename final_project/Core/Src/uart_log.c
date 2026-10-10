/**
 * @file    uart_log.c
 * @brief   執行緒安全的 UART log 模組（Person B 負責）
 *
 * 修正說明（相對於 pubsub_design.md 的骨架）：
 *  1. 補齊 STM32 HAL 相依：使用 huart2（CubeMX 預設），extern 宣告。
 *  2. Scheduler 啟動前（log_mutex 還不存在）仍可安全呼叫：直接 HAL 輸出。
 *  3. HAL_UART_Transmit 加上合理 timeout（100ms）而非 HAL_MAX_DELAY，
 *     防止 UART 硬體故障卡死整個系統。
 */

#include "uart_log.h"
#include "main.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * 如果 CubeMX 使用不同的 UART（例如 USART1），修改此處
 * ----------------------------------------------------------------------- */
extern UART_HandleTypeDef huart2;
#define LOG_UART  (&huart2)
#define LOG_UART_TIMEOUT_MS  100

/* -----------------------------------------------------------------------
 * Module-private state
 * ----------------------------------------------------------------------- */
static SemaphoreHandle_t log_mutex = NULL;
static char              log_buf[256];

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void uart_log_init(void)
{
    /* UART HW 由 main.c 的 MX_USART2_UART_Init() 初始化，這裡只建 mutex */
    log_mutex = xSemaphoreCreateMutex();
    configASSERT(log_mutex != NULL);
}

void uart_logf(const char *fmt, ...)
{
    va_list args;
    int n;

    va_start(args, fmt);
    n = vsnprintf(log_buf, sizeof(log_buf), fmt, args);
    va_end(args);

    if (n <= 0) return;
    if (n >= (int)sizeof(log_buf)) {
        n = sizeof(log_buf) - 1;  /* 截斷，確保不越界 */
    }

    if (log_mutex == NULL) {
        /* Scheduler 尚未啟動，直接輸出（無競爭風險） */
        HAL_UART_Transmit(LOG_UART, (uint8_t *)log_buf, (uint16_t)n,
                          LOG_UART_TIMEOUT_MS);
    } else {
        /*
         * ★ 注意：uart_logf 不可在持有 topic_mutex 的情況下呼叫，
         *   否則 topic_mutex + log_mutex 巢狀鎖定可能死鎖。
         *   pubsub.c 已修正此問題（publish 在 mutex 外才呼叫 uart_logf）。
         */
        if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(LOG_UART_TIMEOUT_MS)) == pdTRUE) {
            HAL_UART_Transmit(LOG_UART, (uint8_t *)log_buf, (uint16_t)n,
                              LOG_UART_TIMEOUT_MS);
            xSemaphoreGive(log_mutex);
        }
        /* 若 mutex 取得 timeout，直接跳過這筆 log（不卡住 task） */
    }
}
