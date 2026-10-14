#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"

uint8_t RxData;


int main(void)
{
	OLED_Init();
    
    Serial_Init();
    
    OLED_ShowString(1, 1, "RxData:");
    
    
    
	while (1)
	{
		//中断接收：Serial_Init 里已开 RXNE 中断，这里查标志位取数据
        if (Serial_GetRxFlag() == 1)
        {
            RxData = Serial_GetRxData();
            OLED_ShowHexNum(1, 8, RxData, 2);
        }
	}
}
