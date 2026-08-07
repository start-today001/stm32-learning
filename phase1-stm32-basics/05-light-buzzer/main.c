#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Buzzer.h"
#include "LightSensor.h"


int main(void)
{
	Buzzer_Init();     //蜂鸣器初始化	
	LightSensor_Init();  	//光敏电阻初始化
	
	while (1)
	{
		if (LightSensor_Get()== 1)   //光敏电阻输入为1
		{
			Buzzer_ON();  //蜂鸣器开
		}
		else
		{
			Buzzer_OFF();   //蜂鸣器关
		}
		
	}
}
