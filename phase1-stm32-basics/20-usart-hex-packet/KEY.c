#include "stm32f10x.h"                  // Device header
#include "Delay.h"

void Key_Init(void)
{
	//开启APB2时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_Initsructure;
	GPIO_Initsructure.GPIO_Mode =GPIO_Mode_IPU ;			//上拉输入
	GPIO_Initsructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;
	GPIO_Initsructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_Initsructure);
}

uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)== 0)
	{
		Delay_ms(20);				//抖动延迟20s
		while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)== 0);
		Delay_ms(20);
		KeyNum = 1;
	}

	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0)
	{
		Delay_ms(20);
		while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0);
		Delay_ms(20);
		KeyNum = 2;
	}
	
	return KeyNum;
}
