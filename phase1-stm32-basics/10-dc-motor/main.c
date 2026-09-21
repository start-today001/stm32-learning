#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "MOTER.h"
#include "KEY.h"

uint8_t KeyNum;
int8_t Speed;


int main(void)
{
	OLED_Init();
	Moter_Init();
	Key_Init();
	
	OLED_ShowString(1, 1, "Speed:");			//oled屏显示speed
	
	while (1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 1)
		{
			Speed += 20;
			//反转函数
			if(Speed > 100)
			{
				Speed = -100;
			}
		}
		Moter_SetSpeed(Speed);
		OLED_ShowSignedNum(1, 7, Speed, 3);
	}
}
