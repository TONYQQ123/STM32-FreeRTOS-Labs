#ifndef DHT11_H
#define DHT11_H

#include "stm32f4xx_hal.h"

typedef struct {
    int8_t  temperature;  /* -20 ~ 60 °C */
    uint8_t humidity;     /* 0 ~ 100 % */
} DHT11_Data;

typedef enum {
    DHT11_OK    = 0,
    DHT11_ERROR = 1,
} DHT11_Status;

/**
 * @brief 初始化 DHT11（啟用 DWT cycle counter 計時）
 */
void DHT11_Init(void);

/**
 * @brief 讀取 DHT11 一次完整資料
 * @param data 輸出：溫度和濕度
 * @return DHT11_OK 或 DHT11_ERROR
 *
 * @note 呼叫後至少等 2 秒再呼叫一次
 */
DHT11_Status DHT11_Read(DHT11_Data *data);

#endif
