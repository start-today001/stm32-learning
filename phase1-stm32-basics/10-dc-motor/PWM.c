#include "stm32f10x.h"                  // Device header


void PWM_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitSturcture;
	GPIO_InitSturcture.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitSturcture.GPIO_Pin=GPIO_Pin_2;
	GPIO_InitSturcture.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitSturcture);
	
	TIM_InternalClockConfig(TIM2);
	
	TIM_TimeBaseInitTypeDef Tim_InitStructure;
	Tim_InitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	Tim_InitStructure.TIM_CounterMode=TIM_CounterMode_Up;
	
	/*
	通过加大频率：通过预分频器孔子（超出人耳听觉范围）以便消除马达的类似于蜂鸣器的声音
	PSC从72改为36 现在为20KHZ
	*/
	Tim_InitStructure.TIM_Period=100 - 1;
	Tim_InitStructure.TIM_Prescaler=36 -1;
	Tim_InitStructure.TIM_RepetitionCounter=0;
	
	TIM_TimeBaseInit(TIM2,&Tim_InitStructure);
	
	TIM_OCInitTypeDef Tim_OCInitStructure;
	TIM_OCStructInit(&Tim_OCInitStructure);			//结构体所有的值默认初始化
	
	
	Tim_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;		//PWM1模式
	Tim_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;				//极性为高电平
	Tim_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
	Tim_OCInitStructure.TIM_Pulse=0x0000;
	TIM_OC3Init(TIM2, &Tim_OCInitStructure);
	
	TIM_Cmd(TIM2, ENABLE);
	
}

	void PWM_SetCompare3(uint16_t Compare)
	{
		TIM_SetCompare3(TIM2, Compare);
	}
