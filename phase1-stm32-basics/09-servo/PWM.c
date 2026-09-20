#include "stm32f10x.h"                  // Device header


void PWM_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//复用推挽输出，把引脚交给外设（这里指PWM）
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	

	
	TIM_InternalClockConfig(TIM2);			//内部时钟
	
	TIM_TimeBaseInitTypeDef TIM_Struct;
	TIM_Struct.TIM_Prescaler=72 - 1;			//PSC
	TIM_Struct.TIM_Period=20000 - 1;			//ARR
	TIM_Struct.TIM_CounterMode=TIM_CounterMode_Up;		//向上计数模式
	TIM_Struct.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_Struct.TIM_RepetitionCounter=0;
	TIM_TimeBaseInit(TIM2, &TIM_Struct);
	
	
	TIM_OCInitTypeDef TIM_OCInitStructture;
	TIM_OCStructInit(&TIM_OCInitStructture);
	TIM_OCInitStructture.TIM_OCMode=TIM_OCMode_PWM1;
	// TIM_OCInitStructture.TIM_OCNPolarity=TIM_OCPolarity_High; TIM2为通用定时器，默认为高
	TIM_OCInitStructture.TIM_OutputState=TIM_OutputState_Enable;
	TIM_OCInitStructture.TIM_Pulse=0;                	//CCR的值
	TIM_OC2Init(TIM2,&TIM_OCInitStructture);
	
	TIM_Cmd(TIM2,ENABLE);
	
	
}

void PWM_SetCompare2(uint16_t Compare)
{
	TIM_SetCompare2(TIM2, Compare);
}
