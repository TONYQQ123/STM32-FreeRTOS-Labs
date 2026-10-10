#ifndef SSD1315_H
#define SSD1315_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* I2C 地址（SA0 接 GND → 0x3C，接 VCC → 0x3D） */
#define SSD1315_ADDR  (0x3C << 1)

#define SSD1315_WIDTH   128
#define SSD1315_HEIGHT   64

/**
 * @brief 初始化 OLED
 */
HAL_StatusTypeDef SSD1315_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief 清除整個畫面（全黑）
 */
void SSD1315_Clear(I2C_HandleTypeDef *hi2c);

/**
 * @brief 在指定位置顯示字串（8×8 像素字型，最多 16 字/行）
 * @param col  欄位 0–15
 * @param row  行  0–7
 * @param str  ASCII 字串
 */
void SSD1315_SetCursor(I2C_HandleTypeDef *hi2c, uint8_t col, uint8_t row);
void SSD1315_WriteString(I2C_HandleTypeDef *hi2c, uint8_t col, uint8_t row, const char *str);

#endif
