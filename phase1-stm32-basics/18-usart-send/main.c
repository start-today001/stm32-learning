#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"




int main(void)
{
	OLED_Init();
    
    Serial_Init();
    
    
   // Serial_SendByte(0x41);          //第一次测试发送一个字符函数
    
    
//   MyArray[] = {0x41, 0x42, 0x43, 0x44};
//    Serial_SendArray(MyArray, 4);
    
    
    //Serial_SendString("HelloWord\r\n");         //\r\n 具体查询ASCII表
    
  //  Serial_SendNumber(12345,5);
    
    
    printf("Num = %d\r\n", 666);
    
    
	while (1)
	{
		
	}
}
