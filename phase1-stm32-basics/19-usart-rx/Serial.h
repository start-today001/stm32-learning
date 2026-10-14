#ifndef __SERIAL__H
#define __SERIAL__H

#include <stdio.h>

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint32_t length);
void Serial_SendString(char *String);
uint32_t Serial_Pow(uint32_t x, uint32_t y);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
uint8_t Serial_GetRxData(void);
uint8_t Serial_GetRxFlag(void);
void USART1_IRQHandler(void);

#endif
