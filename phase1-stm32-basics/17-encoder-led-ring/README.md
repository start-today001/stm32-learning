# 自制练习：编码器控制环形流水灯

> **这不是江科大教程里的一集**（接线图目录里没有这个编号），是根据 6-8 编码器接口 + 3-x LED 自己出的综合填空练习。
> ⚠️ 原工程文件夹名叫 `PWM控制led流水灯`，**与内容不符**（本工程没有用 PWM，也不是简单的流水灯）—— 仓库目录名改为 `17-encoder-led-ring`，按实际内容命名。

## 硬件
- MCU: STM32F103C8T6
- **6 颗 LED**: PA0 ~ PA5（推挽输出 `GPIO_Mode_Out_PP`，**低电平点亮**）
- **旋转编码器 A/B 相**: PA6 / PA7（上拉输入，即 TIM3_CH1 / TIM3_CH2）
- OLED: SCL = PB8、SDA = PB9（软件模拟 I2C）

## 功能
- 转动编码器 → 6 颗 LED 上的"亮点"跟着移动（环形，绕圈）
- 正转往一个方向走，**反转往回走**（位置可正可负）
- OLED 第 1 行显示累计原始计数值 `Pos:`，方便观察负数和回绕

## 实现原理

### 1. 编码器增量 → 累计位置
```c
int16_t pos_raw = 0;
...
pos_raw += Encoder_Get();     /* ★4 */
```
- `Encoder_Get()` 内部是「**读 `CNT` → 把 `CNT` 写 0**」，返回的是**自上次调用以来转过的计数**（增量，可正可负）
- 所以位置必须**自己累加**出来 —— 这一步不需要判断方向，正转给正数、反转给负数

### 2. 4 倍频换算
```c
step1 = pos_raw / 4 % LED_NUM;
```
- 编码器接口用的是 `TIM_EncoderMode_TI12`（**TI1、TI2 两相的边沿都计数**），所以转一格产生 **4 个计数**
- 除以 4 才是一个"格"的位置

### 3. 环形回绕（本练习唯一的真坑）
```c
step1 = pos_raw / 4 % LED_NUM;
step2 = step1 + LED_NUM;
pos   = step2 % LED_NUM;      /* pos 恒落在 0 ~ LED_NUM-1 */
```
**为什么要加一次 `LED_NUM` 再取余？** 因为 **C 语言里 `%` 对负数返回负数**：

| `pos_raw/4` | `step1 = x % 6` | `step2 = step1 + 6` | `pos = step2 % 6` |
|---|---|---|---|
| 7 | 1 | 7 | **1** ✅ |
| -1 | **-1** | 5 | **5** ✅ |
| -7 | **-1** | 5 | **5** ✅ |
| -25 | **-1** | 5 | **5** ✅ |

如果**不加** `LED_NUM` 直接 `pos = step1`：`step1` 会是 -1，而 `pos` 是 `uint8_t` → **-1 变成 255** → `1 << 255` 传给 `GPIO_ResetBits`（参数是 `uint16_t`）→ **未定义行为**，表现就是灯乱亮、或转到负位置时灯全灭。

## 学习点

### 1. ★ 为什么要「不调用 Timer_Init()」—— `[WEAK]` 默认中断服务函数是死循环
`main.c` 里特意写了这句警告，**这是本工程最值得记的知识点**：

```c
/* 注意：第 1 步不要调用 Timer_Init()。
   因为覆盖 main.c 后 TIM2 的中断服务函数没了，中断一开就跑到默认死循环。 */
```
- `system/Timer.c` 里的 `Timer_Init()` 做了 `TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE)` + `NVIC_Init(TIM2_IRQn, ENABLE)`，**但这个工程里没有写 `TIM2_IRQHandler`**
- 启动文件 `startup_stm32f10x_md.s` 里，所有中断向量都有 `[WEAK]` 标记的**默认实现** —— 它们**共用末尾一条原地跳转指令**（本仓库 `lib/start/startup_stm32f10x_md.s`）：
  ```asm
  183:  Default_Handler PROC
  184:                  EXPORT  WWDG_IRQHandler   [WEAK]
        ...
  257:  TIM2_IRQHandler            ; ← 这个标签本身没有指令，直接往下"穿透"
  258:  TIM3_IRQHandler
        ...
  273:                  B       .   ; ← 所有默认处理函数最终都落到这一条：原地死循环
  275:                  ENDP
  ```
  关键是 **`B .`（branch to self）** —— 跳到自己，永远出不来
- 所以：**开了中断却没有自己的 ISR → CPU 跳进默认处理函数 → 卡死在死循环**
- 结论：**中断使能必须和中断服务函数成对出现**。这不是"没写就什么都不发生"，而是"没写就直接卡死"

### 2. 位掩码：`1 << pos` 与 `GPIO_ResetBits` 的参数语义
```c
GPIO_ResetBits(GPIOA, 1 << pos);
```
- `GPIO_ResetBits(端口, 引脚掩码)` 第二个参数要的是「**哪几位**」，不是「第几位」
- 所以只有第 `pos` 位为 1 的掩码 = `1 << pos`
- 参数类型是 `uint16_t` —— `pos` 超过 15 就是 UB（见上文的 255 陷阱）

### 3. 硬件宏：引脚/端口只在一处定义
```c
#define LED_NUM     6        /* LED.h */
#define LED_PINS    (GPIO_Pin_0|GPIO_Pin_1|...|GPIO_Pin_5)   /* LED.c */
```
- 改灯数量时**只改这两处**，不用挨个函数找
- `LED.h` 里 `#include <stdint.h>` 用的是标准头（不依赖 `stm32f10x.h`），这样头文件可以独立使用 —— **比之前的项目规范**

## 已知边界（诚实标注 · 全部为"记进 README 不改代码"的取舍）

- **★1 ~ ★5 的题面注释全部保留在代码里**（`LED.c` 3 处、`main.c` 2 处），包含了提示、警告、`>>> 在这里写一行 <<<` 等练习脚手架。练习已做完，这些注释属于**历史题面**
- **★3（可选）未完成**：`LED.c` 第 50 行要求「在下面写一句注释，说明你的做法：在 main 里保证 pos 合法，还是这里加防御判断」—— **没有写**
- **`LED_PORT` 宏没有贯彻**：`LED.c` 第 5 行定义了 `#define LED_PORT GPIOA`，注释写着「引脚/端口只在这里改一次，别散落在函数里」，但第 30 行 `GPIO_SetBits(GPIOA, ...)`、第 48 行 `GPIO_ResetBits(GPIOA, ...)` **仍然硬编码 `GPIOA`** —— 宏的意义没实现（真要换端口还得改 2 处）
- **`Timer.c / Timer.h` 纳入了仓库但实际未被调用**：`main.c` 故意不调 `Timer_Init()`（原因见学习点 1）。之所以仍然纳入，是因为 **`main.c` 的注释直接引用它**来解释那个陷阱 —— 不上传的话这个知识点就断了。按"只传用到的文件"的惯例它们本不该传，这里是有意例外
- **文件行尾不统一**：`main.c` 与 `LED.c` 是 **LF** 换行，其余文件是 **CRLF**。Keil 两者都能编译，git 提交时也会归一化 —— 纯粹的一致性问题
- **工作区文件夹名与内容不符**：原为 `PWM控制led流水灯`，但工程里没有使用 PWM。仓库目录名用 `17-encoder-led-ring`
- `KEY.c/h` 未被任何文件 include，未纳入
- `Timer.h` 的保护宏是 `__Timer_H`（混合大小写，仓库其他文件是全大写；且下划线开头属保留标识符）
- `Encoder.c/h` 与已上传的 `13-encoder-speed` **逐字节相同**（同一份驱动复用）；`OLED` / `Delay` / `stm32f10x_conf.h` / `stm32f10x_it.c/.h` 与 `16-dma-transfer` 相同

## 文件
- `main.c` — 主程序（累加编码器增量 → 环形映射 → 点亮对应 LED + OLED 显示原始值）
- `LED.c / LED.h` — 6 颗 LED 驱动（`LED_NUM` 宏 + `LED_OffAll` + `LED_ShowOne`）
- `Encoder.c / Encoder.h` — 编码器接口驱动（与 `13-encoder-speed` 相同）
- `Timer.c / Timer.h` — TIM2 秒中断配置（**本工程故意不调用**，见学习点 1）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
