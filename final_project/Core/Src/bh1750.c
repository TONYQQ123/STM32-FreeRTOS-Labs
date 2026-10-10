#include "bh1750.h"

#define BH1750_POWER_ON       0x01
#define BH1750_RESET          0x07
#define BH1750_CONT_H_RES     0x10  /* 連續高解析度模式，1 lux 精度，120ms */

HAL_StatusTypeDef BH1750_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t cmd;

    cmd = BH1750_POWER_ON;
    if (HAL_I2C_Master_Transmit(hi2c, BH1750_ADDR, &cmd, 1, 100) != HAL_OK)
        return HAL_ERROR;
    HAL_Delay(10);

    cmd = BH1750_CONT_H_RES;
    if (HAL_I2C_Master_Transmit(hi2c, BH1750_ADDR, &cmd, 1, 100) != HAL_OK)
        return HAL_ERROR;
    HAL_Delay(180);  /* 等待第一次量測完成 */

    return HAL_OK;
}

HAL_StatusTypeDef BH1750_Read(I2C_HandleTypeDef *hi2c, uint16_t *lux)
{
    uint8_t buf[2] = {0};

    if (HAL_I2C_Master_Receive(hi2c, BH1750_ADDR, buf, 2, 100) != HAL_OK)
        return HAL_ERROR;

    /* 原始值 / 1.2 = lux */
    uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
    *lux = (uint16_t)(raw / 1.2f);
    return HAL_OK;
}
