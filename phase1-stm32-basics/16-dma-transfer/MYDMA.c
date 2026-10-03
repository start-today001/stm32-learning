#include "stm32f10x.h"                  // Device header

uint16_t MYDMA_Size;


/*
对于SRAM的数组，地址由编译器分配，不固定
这里改用数组名来确认地址
*/
void MYDMA_Init(uint32_t AddrA, uint32_t AddrB, uint16_t Size)			
{
	MYDMA_Size = Size;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_PeripheralBaseAddr=AddrA;
	DMA_InitStructure.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralInc=DMA_PeripheralInc_Enable;
	DMA_InitStructure.DMA_MemoryBaseAddr=AddrB;
	DMA_InitStructure.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc=DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_DIR=DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize=Size;
	DMA_InitStructure.DMA_Mode=DMA_Mode_Normal;
	DMA_InitStructure.DMA_M2M=DMA_M2M_Enable;
	DMA_InitStructure.DMA_Priority=DMA_Priority_Medium;
	
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	
	//DMA_Cmd(DMA1_Channel1, ENABLE);
	DMA_Cmd(DMA1_Channel1, DISABLE);		//灯调用tranfer函数后再调用
}

//关于DATA的值变化，继续转运
void MYDMA_Transfer(void)
{
	DMA_Cmd(DMA1_Channel1, DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel1, MYDMA_Size);
	DMA_Cmd(DMA1_Channel1, ENABLE);
	
	while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);
	DMA_ClearFlag(DMA1_FLAG_TC1);
	
}
