#include "stm32f10x.h"                  // Device header


void IC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);			//TIM2用来输出PWM波形，改成TIM3
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//通过引脚定义表，确认引脚
	
	
	
	GPIO_InitTypeDef GPIO_Initstructure;
	GPIO_Initstructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Initstructure.GPIO_Pin=GPIO_Pin_6;
	GPIO_Initstructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init (GPIOA, &GPIO_Initstructure);
	
	
	TIM_InternalClockConfig(TIM3);
	
	TIM_TimeBaseInitTypeDef Tim_InitStructure;
	Tim_InitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	Tim_InitStructure.TIM_CounterMode=TIM_CounterMode_Up;
	Tim_InitStructure.TIM_Period=65536 - 1;				//改为最大值，防止计数器溢出
	Tim_InitStructure.TIM_Prescaler=72 - 1;			
	Tim_InitStructure.TIM_RepetitionCounter=0;
	TIM_TimeBaseInit(TIM3,&Tim_InitStructure);
	
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICInitStructure.TIM_Channel=TIM_Channel_1;			//选择通道
	TIM_ICInitStructure.TIM_ICFilter=0x0F;					//滤波器（使信号更加平滑，抗干扰）的频率高于分频器的频率
	TIM_ICInitStructure.TIM_ICPolarity=TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICPrescaler=TIM_ICPSC_DIV1;		//分频器
	TIM_ICInitStructure.TIM_ICSelection=TIM_ICSelection_DirectTI;
	TIM_ICInit(TIM3,&TIM_ICInitStructure);
	

	
	//从模式
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);
	
	TIM_Cmd(TIM3, ENABLE);
}

void IC_PWMI_Init(void)
{
	//通道2pwmi测同一个引脚的pwm
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICInitStructure.TIM_Channel=TIM_Channel_2;			
	TIM_ICInitStructure.TIM_ICFilter=0x0F;					
	TIM_ICInitStructure.TIM_ICPolarity=TIM_ICPolarity_Falling;
	TIM_ICInitStructure.TIM_ICPrescaler=TIM_ICPSC_DIV1;		
	TIM_ICInitStructure.TIM_ICSelection=TIM_ICSelection_IndirectTI;			//交叉输入
	TIM_ICInit(TIM3,&TIM_ICInitStructure);
	
	
	// TIM_PWMIConfig(TIM3, &TIM_ICInitStructure);				//自动配置PWMI，相当于上面注释掉的代码
}

uint32_t IC_GetFreq(void)
{
	return 1000000 / (TIM_GetCapture1(TIM3) + 1);
	//有误差1，+1看起来无误差
}

uint32_t IC_GetDuty(void)
{
	return TIM_GetCapture2(TIM3) * 100 / TIM_GetCapture1(TIM3);
}
