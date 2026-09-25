#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "IC.h"


int main(void)
{
	OLED_Init();
	PWM_Init();
	IC_Init();
	IC_PWMI_Init();				//6-7 需要：额外配好通道2（PWMI）才能测占空比
	
	OLED_ShowString(1, 1, "Freq:00000HZ");
	OLED_ShowString(2, 1, "Duty:00%");
	
	PWM_SetPrescaler(720 -1);		//Freq = 72M / (PSC + 1) / 100
	PWM_SetCompare1(50);			//Duty = CCR / 100
	
	//Duty = CCR / 100  			ARR = 100
	while (1)
	{
	OLED_ShowNum(1, 6, IC_GetFreq(), 5);
		//有误差1,为什么有误差
	OLED_ShowNum(2, 6, IC_GetDuty(), 2);
	}
}
