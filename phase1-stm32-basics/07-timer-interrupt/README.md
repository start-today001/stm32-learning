# 6-1 定时器中断和外部时钟

## 硬件
- MCU: STM32F103C8T6
- OLED SCL: PB8（开漏输出，软件模拟 I2C 时钟线）
- OLED SDA: PB9（开漏输出，软件模拟 I2C 数据线）
- 定时器: TIM2（挂在 APB1 总线，内部时钟 72MHz）

## 功能
- OLED 第一行显示 `Num:` 加变量 Num 的值
- OLED 第二行显示 TIM2 当前计数值 `TIM_GetCounter(TIM2)`

## 定时器配置（Timer.c）
- `RCC_APB1PeriphClockCmd`：TIM2 在 APB1 上（TIM1 在 APB2，别搞混）
- 预分频 `TIM_Prescaler = 7200 - 1`：72MHz ÷ 7200 = **10kHz**，每 0.1ms 计一个数
- 自动重装 `TIM_Period = 10000 - 1`：计满 10000 个数溢出一次 → 10kHz ÷ 10000 = **每 1 秒溢出一次**
- 为什么两个值都要减 1：寄存器里的值 + 1 才是实际的分频系数 / 计数上限，写 0 表示不分频
- `TIM_InternalClockConfig(TIM2)`：选择内部时钟源（对应教程标题里的「外部时钟」是另一种配置方式，本项目用的是内部时钟）

## NVIC 中断配置
- `NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2)`：分组 2 = 2 位抢占优先级 + 2 位响应优先级
- `NVIC_IRQChannel = TIM2_IRQn`：中断通道号，必须和 startup 文件向量表里的名字对应上
- 抢占优先级 2 / 响应优先级 1

## 学习点
- 定时器初始化的四步顺序：开时钟 → 配置时基单元 → 配置中断（`TIM_ITConfig` + `NVIC_Init`）→ `TIM_Cmd` 使能
- 为什么 `TIM_ClearFlag(TIM2, TIM_FLAG_Update)` 要放在 `TIM_ITConfig` 之前：初始化时可能已经产生过一次更新标志，不先清掉的话，一使能中断就会立刻误进一次
- `NVIC_PriorityGroupConfig` 全局只能设置一次，通常放在 main 或第一个初始化的外设里
- 中断产生的完整链路：更新事件（溢出）→ 硬件置标志位 → 标志位 + 中断使能都满足 → NVIC 转发 → 跳转中断服务函数

## 已知边界（重要）
- **本工程的定时器中断服务函数 `TIM2_IRQHandler` 目前是注释掉的**（见 `Timer.c` 第 39~49 行）
- 所以 `Num` 变量**不会自增**，OLED 第一行恒为 0；第二行能看到 TIM2 计数器在 0~9999 之间循环（约 1 秒一轮）
- 也就是说，**当前版本演示的是「轮询读定时器计数器」，不是「定时器中断」**
- 中断部分待补：把 `/* */` 去掉即可生效（括号写法已修正）

## 调试记录（踩过的坑）
- 原写法 `TIM_GetITStatus(TIM2, TIM_IT_Update == SET)` —— **右括号位置错了**，正确是 `TIM_GetITStatus(TIM2, TIM_IT_Update) == SET`
- 这个错写法**侥幸能跑**：`TIM_IT_Update` 的值是 `0x0001`，`SET` 的值是 `1`，所以 `TIM_IT_Update == SET` 求值成 `1`，传进去碰巧等于 `TIM_IT_Update` 本尊
- 但这是"凑巧等价"，不是"正确"：换个枚举值就出错，而且面试官看到会认为没理解括号的作用域
- 教训：比较运算符一定要写在**整个函数调用之外**，别塞进参数列表里

## 文件
- `main.c` — 主程序（初始化 + 循环刷新 Num 和 TIM2 计数值）
- `Timer.c / Timer.h` — TIM2 时基配置 + NVIC 中断配置（中断服务函数当前已注释）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
