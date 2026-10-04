#ifndef __LED_H
#define __LED_H

#include <stdint.h>

/* 一共几颗灯。改灯数量时只改这一处 + LED.c 里的 LED_PINS */
#define LED_NUM     6

void LED_Init(void);
void LED_OffAll(void);
void LED_ShowOne(uint8_t pos);      /* 只点亮第 pos 颗，其余全灭；pos = 0 ~ LED_NUM-1 */

#endif
