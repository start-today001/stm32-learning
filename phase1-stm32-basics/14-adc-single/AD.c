#include "stm32f10x.h"                  // Device header


void AD_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);		//配置分频器,ADC不得超过14MHZ，所以选择使得72除以分频数不得大于14MHZ的
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AIN;		//AIN模式，GPIO输入输出无效，防止干扰
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//ADC初始化
	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode=ADC_Mode_Independent;		//独立模式
	ADC_InitStructure.ADC_ContinuousConvMode=DISABLE;		//转换模式
	ADC_InitStructure.ADC_DataAlign=ADC_DataAlign_Right;		//数据对齐
	ADC_InitStructure.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;		//外部触发源
	ADC_InitStructure.ADC_NbrOfChannel=1;				//扫描模式下，通道数
	ADC_InitStructure.ADC_ScanConvMode=DISABLE;			//选择扫描模式
	ADC_Init(ADC1, &ADC_InitStructure);
	
	
	//通道号和采样时间
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
	
	
	
	ADC_Cmd(ADC1, ENABLE);			//开启电源
	
	//校准模式
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1) ==  SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
		
}

uint16_t AD_GetValue(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	return ADC_GetConversionValue(ADC1);
}
