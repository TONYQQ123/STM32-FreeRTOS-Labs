#include "dht11.h"

/* DHT11 接在 PC0 */
#define DHT11_PORT  GPIOC
#define DHT11_PIN   GPIO_PIN_2

/* DWT 微秒延遲（比 HAL_Delay 精確，HAL_Delay 最小單位是 1ms） */
static inline void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks);
}

static void pin_output(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = DHT11_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_PORT, &g);
}

static void pin_input(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin  = DHT11_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &g);
}

static inline uint8_t pin_read(void)
{
    return HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET ? 1 : 0;
}

void DHT11_Init(void)
{
    /* 啟用 DWT */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    __HAL_RCC_GPIOC_CLK_ENABLE();
    pin_output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
}

DHT11_Status DHT11_Read(DHT11_Data *data)
{
    uint8_t raw[5] = {0};

    /* ---- 起始訊號：拉低 18ms，再拉高 ---- */
    pin_output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);

    /* ---- 切換為輸入，等待 DHT11 回應 ---- */
    pin_input();

    /* 等待低電位（DHT11 拉低 80us） */
    uint32_t timeout = 10000;
    while (pin_read() == 1) { if (--timeout == 0) return DHT11_ERROR; }
    /* 等待高電位（DHT11 拉高 80us） */
    timeout = 10000;
    while (pin_read() == 0) { if (--timeout == 0) return DHT11_ERROR; }
    timeout = 10000;
    while (pin_read() == 1) { if (--timeout == 0) return DHT11_ERROR; }

    /* ---- 讀取 40 bits（5 bytes） ---- */
    for (int i = 0; i < 40; i++) {
        /* 等低電位結束（50us 前導） */
        timeout = 10000;
        while (pin_read() == 0) { if (--timeout == 0) return DHT11_ERROR; }

        /* 計算高電位持續時間：26-28us = 0，70us = 1 */
        delay_us(40);
        raw[i / 8] <<= 1;
        if (pin_read() == 1) {
            raw[i / 8] |= 1;
            /* 等高電位結束 */
            timeout = 10000;
            while (pin_read() == 1) { if (--timeout == 0) return DHT11_ERROR; }
        }
    }

    /* ---- 校驗碼 ---- */
    if (raw[4] != (uint8_t)(raw[0] + raw[1] + raw[2] + raw[3]))
        return DHT11_ERROR;

    data->humidity    = raw[0];
    data->temperature = (int8_t)raw[2];
    return DHT11_OK;
}
