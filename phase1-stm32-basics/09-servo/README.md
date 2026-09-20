# 6-4 舵机驱动（PWM 控制角度）

## 硬件
- MCU: STM32F103C8T6
- 舵机信号线: PA1（复用推挽输出 `GPIO_Mode_AF_PP`，对应 TIM2 通道 2）
- 按键: PB1 / PB11（上拉输入 `GPIO_Mode_IPU`，按下为低电平）
- OLED: SCL = PB8、SDA = PB9（软件模拟 I2C）

## 功能
- 按 PB1 → 舵机角度 +30°，超过 180° 后归 0
- OLED 第 1 行显示当前角度

## 舵机 PWM 时序（PWM.c）
- 时钟：TIM2 挂 APB1（`RCC_APB1Periph_TIM2`），GPIOA 挂 APB2
- 引脚 PA1 必须配 `GPIO_Mode_AF_PP`（复用推挽）——把引脚控制权交给定时器，不是普通 GPIO 输出
- 预分频 `72 - 1`：72MHz ÷ 72 = **1MHz** → 1 个计数 = 1µs
- 自动重装 `20000 - 1`：20000µs = **20ms** → 频率 50Hz（舵机标准周期）
- 通道与引脚必须对应：PA1 = TIM2_CH2，所以用 `TIM_OC2Init` / `TIM_SetCompare2`
- 定时器频率公式：**72MHz ÷ (PSC+1) ÷ (ARR+1)** —— 先定频率，再定分辨率

## 角度 → 脉宽的换算（Servo.c）
```c
Servo_SetAngle(Angle) → CCR = Angle / 180 * 2000 + 500
```
| 角度 | CCR | 脉宽 |
|---|---|---|
| 0° | 500 | 0.5ms |
| 90° | 1500 | 1.5ms |
| 180° | 2500 | 2.5ms |

即「0.5~2.5ms 脉宽对应 0~180°」，SG90 这类舵机的标准映射。舵机内部按脉宽决定转角，所以**改 CCR 就是改角度**。

## 学习点
- 同一个 PWM 单元，两种完全不同的用法：**6-3 调 LED 亮度**（改平均功率）、**6-4 控舵机角度**（脉宽本身就是指令）。PWM 在舵机这里不是"调节量"，而是"编码值"
- `TIM_OCStructInit()` 的作用：先把输出比较结构体填成安全默认值（`TIM_OCPolarity` 默认 High、`TIM_OutputState` 默认 Disable），再按需覆盖，避免结构体里的野值
- 按键消抖流程：检测按下 → `Delay_ms(20)` → 等服务释放 → `Delay_ms(20)`
- 浮点参与运算的代价：`Angle` 用 `float` 会引入浮点运算，这里其实 `uint8_t` 就够

## 已知边界
- `main.c` 用 `float Angle` 累加，但 `OLED_ShowNum` 形参是 `uint32_t`，传入时会隐式取整；角度是 30 的整数倍，用整数类型即可，可省掉浮点
- 按键消抖和角度更新都在 `while(1)` 里用 `Delay_ms` 阻塞，按下期间主循环被占死。真实项目应改用定时器扫描或 RTOS 任务（阶段二要解决的问题）
- 头文件大小写：Windows 不区分 `KEY.h` / `Key.h`，但 Linux/Mac 严格区分。仓库内统一用 `KEY.h`

## 调试记录（踩过的坑）
本工程从 6-1 的 `PWM.c` 复制改造而来，改完发现自己踩了 **4 处「写错了但碰巧能跑」** 的坑，已全部修正：

| 原写法 | 错在哪 | 为什么当时没出问题 | 已改成 |
|---|---|---|---|
| `TIM_OCInitStructture.TIM_OCNPolarity = TIM_OCPolarity_High` | 写成了 **N 互补通道**的极性字段；TIM2 是通用定时器，要设的是 `TIM_OCPolarity` | `TIM_OCStructInit()` 已把 `TIM_OCPolarity` 默认设为 `TIM_OCPolarity_High`，值恰好相同 | 删除该行（默认值已正确） |
| `TIM_OCInitStructture.TIM_OutputState = ENABLE` | 类型用错，枚举名应为 `TIM_OutputState_Enable` | `ENABLE` = 1、`TIM_OutputState_Enable` = 0x0001，数值恰好相等 | `TIM_OutputState_Enable` |
| `TIM_ClearFlag()` + `TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE)` | 打开了更新中断，但没有 `NVIC_Init`，也没有 `TIM2_IRQHandler` | 中断源开了但 NVIC 没使能，CPU 不会响应 → 无实际影响 | 删除这两行 |
| `TIM_InternalClockConfig(TIM2); //外部时钟` | 注释和实际调用的函数相反 | 只是注释，不影响编译 | `//内部时钟` |

**教训**：从上一个工程复制代码时，最容易出问题的**不是逻辑，而是「名字和值」**——枚举名、结构体字段名、头文件大小写。

这类错误的特点是：**编译器不报错、警告也不出、行为还正常**。所以只能靠在写的时候对着库头文件（`stm32f10x_tim.h`）逐个核对，或者事后 code review 才发现。

## 文件
- `main.c` — 主程序（按键加角度 + OLED 显示）
- `Servo.c / Servo.h` — 舵机驱动（角度→CCR 换算，内部调用 `PWM_Init`）
- `PWM.c / PWM.h` — TIM2_CH2 PWM 输出（50Hz，PA1）
- `KEY.c / KEY.h` — 按键驱动（PB1 / PB11，上拉输入 + 20ms 消抖）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
