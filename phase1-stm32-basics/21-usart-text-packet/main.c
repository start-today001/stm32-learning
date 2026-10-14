#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "LED.h"
#include <string.h>



int main(void)
{
	OLED_Init();
    
    Serial_Init();
    LED_Init();
    
    OLED_ShowString(1, 1, "TxData:");
    OLED_ShowString(3, 1, "RxData:");
    

 
    
	while (1)
	{
        if (Serial_GetRxFlag() == 1)
        {
            OLED_ShowString(3, 9, "        ");
            OLED_ShowString(3, 9, Serial_RxPacket);
            
            
          if (  strcmp(Serial_RxPacket, "Led_ON") == 0)
          {
              LED1_ON();
              Serial_SendString("LED_ON_OK\r\n");
          }
          else if (  strcmp(Serial_RxPacket, "Led_OFF") == 0)
          {
              LED1_OFF();
              Serial_SendString("LED_OFF_OK\r\n");
          }
        }
            
	}
}
