# 3-2 LED 流水灯

## 硬件
- MCU: STM32F103C8T6
- LED 引脚: PA0 / PA1 / PA2（推挽输出，低电平点亮）
- 现象: 三灯依次点亮，200ms 切换，形成流水效果

## 学习点
- GPIO 多引脚统一配置（`GPIO_Pin_0 | GPIO_Pin_1 | ...`）
- `GPIO_SetBits` / `GPIO_ResetBits` 逐灯控制
- 宏定义 LEDx_PIN 避免硬编码

## 文件
- `main.c` — 主程序（流水灯轮询逻辑）
