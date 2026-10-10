#ifndef UART_LOG_H
#define UART_LOG_H

#include <stdarg.h>

/**
 * @brief 初始化 UART log 模組（建立 log_mutex）。
 *        必須在 scheduler 啟動前、且在 MX_USARTx_UART_Init() 之後呼叫。
 */
void uart_log_init(void);

/**
 * @brief 執行緒安全的格式化 UART 輸出，用法同 printf。
 *        在 scheduler 啟動前仍可安全呼叫（mutex 還不存在時會直接 HAL 輸出）。
 */
void uart_logf(const char *fmt, ...);

#endif /* UART_LOG_H */
