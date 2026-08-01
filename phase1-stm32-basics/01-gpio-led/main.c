/**
 * @file    main.c
 * @brief   3-1 LED闪烁：PA0 接 LED，500ms 周期闪烁
 *          学习目标：GPIO 推挽输出 + 软件延时
 */

#include "stm32f10x.h"                  // STM32F10x 标准外设库
#include "Delay.h"                      // 软件延时函数 Delay_ms()

/* 硬件映射 */
#define LED_PORT            GPIOA
#define LED_PIN             GPIO_Pin_0

/* 外设时钟 */
#define LED_RCC             RCC_APB2Periph_GPIOA
#define LED_RCC_CMD         RCC_APB2PeriphClockCmd

/* 参数常量 */
#define BLINK_PERIOD_MS     500         // 闪烁周期（毫秒）

int main(void)
{
    /* 使能 GPIOA 时钟 — APB2 总线，不使能外设无法工作 */
    LED_RCC_CMD(LED_RCC, ENABLE);

    /* 配置 PA0 为推挽输出 — 能直接驱动 LED，不需要外部上拉 */
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.GPIO_Pin   = LED_PIN;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &gpio_init);

    /* 初始状态：LED 熄灭 */
    GPIO_ResetBits(LED_PORT, LED_PIN);

    while (1)
    {
        GPIO_SetBits(LED_PORT, LED_PIN);       // 点亮
        Delay_ms(BLINK_PERIOD_MS);
        GPIO_ResetBits(LED_PORT, LED_PIN);     // 熄灭
        Delay_ms(BLINK_PERIOD_MS);
    }
}
