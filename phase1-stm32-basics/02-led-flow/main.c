/**
 * @file    main.c
 * @brief   3-2 LED 流水灯：PA0/PA1/PA2 三灯轮询点亮
 *          学习目标：GPIO 多引脚控制 + 宏定义数组
 */

#include "stm32f10x.h"                  // STM32F10x 标准外设库
#include "Delay.h"                      // 软件延时函数 Delay_ms()

/* 硬件映射 */
#define LED_PORT            GPIOA
#define LED1_PIN            GPIO_Pin_0
#define LED2_PIN            GPIO_Pin_1
#define LED3_PIN            GPIO_Pin_2
#define LED_ALL             (LED1_PIN | LED2_PIN | LED3_PIN)

/* 外设时钟 */
#define LED_RCC             RCC_APB2Periph_GPIOA

/* 参数常量 */
#define FLOW_DELAY_MS       200         // 切换间隔（毫秒）

int main(void)
{
    /* 使能 GPIOA 时钟 — APB2 总线，不使能外设无法工作 */
    RCC_APB2PeriphClockCmd(LED_RCC, ENABLE);

    /* 配置 PA0/1/2 为推挽输出 — 3 个 LED 共用一个端口 */
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.GPIO_Pin   = LED_ALL;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &gpio_init);

    /* 初始状态：全部熄灭（低电平点亮则全部置高） */
    GPIO_SetBits(LED_PORT, LED_ALL);

    while (1)
    {
        GPIO_ResetBits(LED_PORT, LED1_PIN);    // 灯 1 亮
        Delay_ms(FLOW_DELAY_MS);
        GPIO_SetBits(LED_PORT, LED1_PIN);      // 灯 1 灭

        GPIO_ResetBits(LED_PORT, LED2_PIN);    // 灯 2 亮
        Delay_ms(FLOW_DELAY_MS);
        GPIO_SetBits(LED_PORT, LED2_PIN);      // 灯 2 灭

        GPIO_ResetBits(LED_PORT, LED3_PIN);    // 灯 3 亮
        Delay_ms(FLOW_DELAY_MS);
        GPIO_SetBits(LED_PORT, LED3_PIN);      // 灯 3 灭
    }
}
