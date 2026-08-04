# 3-4 按键控制 LED

## 硬件
- MCU: STM32F103C8T6
- LED1: PA1（推挽输出，低电平点亮）
- LED2: PA2（推挽输出，低电平点亮）
- KEY1: PB1（上拉输入，按下为低电平）
- KEY2: PB11（上拉输入，按下为低电平）

## 功能
- 按下 KEY1 → LED1 翻转
- 按下 KEY2 → LED2 翻转

## 学习点
- GPIO 输入模式配置（GPIO_Mode_IPU 上拉输入）
- 按键消抖：检测按下 → 延时 20ms → 等待释放 → 延时 20ms
- 电平翻转的两种写法：`GPIO_ReadOutputDataBit` 读当前输出状态再取反
- 模块化编程：LED / KEY 拆分为独立驱动文件，main 只做业务逻辑

## 文件
- `main.c` — 主程序（按键扫描 + LED 控制）
- `LED.c / LED.h` — LED 驱动（初始化、开关、翻转）
- `KEY.c / KEY.h` — 按键驱动（初始化、消抖扫描）
- `Delay.c / Delay.h` — 软件延时
