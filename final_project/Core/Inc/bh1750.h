#ifndef BH1750_H
#define BH1750_H

#include "stm32f4xx_hal.h"

/* GY-302 ADDR 腳浮空 → I2C 地址 0x23，接 VCC → 0x5C */
#define BH1750_ADDR  (0x23 << 1)

/**
 * @brief 初始化 BH1750，設定連續高解析度模式
 * @param hi2c I2C handle（&hi2c1）
 * @return HAL_OK 或 HAL_ERROR
 */
HAL_StatusTypeDef BH1750_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief 讀取光照強度
 * @param hi2c  I2C handle
 * @param lux   輸出：照度（勒克斯）
 * @return HAL_OK 或 HAL_ERROR
 */
HAL_StatusTypeDef BH1750_Read(I2C_HandleTypeDef *hi2c, uint16_t *lux);

#endif
