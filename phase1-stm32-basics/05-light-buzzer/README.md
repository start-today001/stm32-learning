# 3-5 光敏电阻控制蜂鸣器

## 硬件
- MCU: STM32F103C8T6
- 光敏电阻: PB13（上拉输入，遮光时输出高电平）
- 蜂鸣器: PB12（推挽输出，低电平触发）

## 功能
- 遮住光敏电阻 → 蜂鸣器响
- 有光照射 → 蜂鸣器停

## 学习点
- 光敏电阻 + 分压电路 → GPIO 数字输入（遮光/见光对应高低电平）
- 输入模式：上拉输入（GPIO_Mode_IPU），读取 `GPIO_ReadInputDataBit`
- 传感器状态轮询（Polling）：`while(1)` 中循环检测，输出控制随输入变化
- 模块化编程：Buzzer / LightSensor 独立驱动文件

## 文件
- `main.c` — 主程序（轮询光敏状态 → 控制蜂鸣器）
- `Buzzer.c / Buzzer.h` — 蜂鸣器驱动（初始化、开关、翻转）
- `LightSensor.c / LightSensor.h` — 光敏电阻驱动（初始化、读取状态）
- `Delay.c / Delay.h` — 软件延时
