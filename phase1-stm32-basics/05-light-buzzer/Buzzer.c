#include "stm32f10x.h"  // Device header



/**
  * 函    数：蜂鸣器初始化
  * 参    数：无
  * 返 回 值：无
  */
  
void Buzzer_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  //开启GPIOB时钟
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);				//将Pin12号接口设置为推挽输出
	
	//默认12号引脚初始化电平
	GPIO_SetBits(GPIOB, GPIO_Pin_12);				//pin12号引脚设置为高电平
	
}



/**
  * 函    数：蜂鸣器状态开启
  * 参    数：无
  * 返 回 值：无
  */
void Buzzer_ON(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);		//将12号引脚设置为低电平
}


/**
  * 函    数：蜂鸣器状关闭
  * 参    数：无
  * 返 回 值：无
  */
void Buzzer_OFF(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_12);		//12号引脚为高电平
}


/**
  * 函    数：蜂鸣器状态翻转
  * 参    数：无
  * 返 回 值：无
  */
void  Buzzer_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_12)== 0)			//获取寄存器的初始状态，若当前为低电平
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_12);						//12号引脚改为高电平
	}
	else														//若当前引脚为高电平
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);						//将12号引脚改为低电平
	}
}
