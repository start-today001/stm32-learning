/**
 * @file    main.c
 * @brief   3-3 蜂鸣器：PB12 驱动有源蜂鸣器，200ms 周期鸣响
 *          学习目标：GPIO 推挽输出驱动外部器件
 */

#include "stm32f10x.h"                  // STM32F10x 标准外设库
#include "Delay.h"                      // 软件延时函数 Delay_ms()

/* 硬件映射 */
#define BUZZER_PORT         GPIOB
#define BUZZER_PIN          GPIO_Pin_12

/* 外设时钟 */
#define BUZZER_RCC          RCC_APB2Periph_GPIOB

/* 参数常量 */
#define BEEP_ON_MS          200         // 鸣响时长（毫秒）
#define BEEP_OFF_MS         200         // 静音时长（毫秒）

int main(void)
{
    /* 使能 GPIOB 时钟 — 蜂鸣器接在 PB12 */
    RCC_APB2PeriphClockCmd(BUZZER_RCC, ENABLE);

    /* 配置 PB12 为推挽输出 — 有源蜂鸣器内含振荡电路，直接 GPIO 驱动即可 */
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.GPIO_Pin   = BUZZER_PIN;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &gpio_init);

    /* 初始状态：蜂鸣器关闭 */
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);

    while (1)
    {
        GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);     // 蜂鸣器响
        Delay_ms(BEEP_ON_MS);
        GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);   // 蜂鸣器停
        Delay_ms(BEEP_OFF_MS);
    }
}
