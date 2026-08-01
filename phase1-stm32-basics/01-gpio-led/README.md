# 3-1 GPIO 输出 — LED 闪烁

## 硬件
- MCU: STM32F103C8T6
- LED 引脚: PA0（推挽输出）
- 现象: LED 以 500ms 周期亮灭

## 学习点
- GPIO 推挽输出配置（GPIO_Mode_Out_PP）
- RCC 外设时钟使能（APB2 总线）
- 软件延时实现周期闪烁
- 为什么要 `RCC_APB2PeriphClockCmd`：上电后外设时钟默认关闭

## 文件
- `main.c` — 主程序（LED 闪烁逻辑 + GPIO 初始化）
