# 6-3 呼吸灯（PWM 输出）

## 硬件
- MCU: STM32F103C8T6
- LED: PA0（复用推挽输出 `GPIO_Mode_AF_PP`，对应 TIM2 的通道 1）
- 定时器: TIM2 通道 1 输出 PWM

## 功能
- LED 由暗渐亮、再由亮渐暗，循环往复（呼吸效果）
- 每级占空比停留 30ms，共 100 级 → 单次渐亮或渐暗约 3 秒

## PWM 配置（PWM.c）
- 时钟：TIM2 在 APB1（`RCC_APB1Periph_TIM2`），GPIOA 在 APB2（`RCC_APB2Periph_GPIOA`）
- 引脚模式必须用 `GPIO_Mode_AF_PP`（复用推挽输出）——引脚交给片上外设控制，不是普通 GPIO 输出
- 预分频 `720 - 1`：72MHz ÷ 720 = **100kHz** 计数频率
- 自动重装 `100 - 1`：100kHz ÷ 100 = **PWM 频率 1kHz**（周期 1ms）
- 占空比 = `CCR / (ARR + 1)` = `CCR / 100`，所以 CCR 取 0~100 就是 0%~100%
- `TIM_OCStructInit(&Tim_OCInitStructure)`：先把输出比较结构体全部填默认值，再改需要的那几项（避免结构体里的野值）
- `TIM_OCMode_PWM1` + `TIM_OCPolarity_High`：CNT < CCR 时输出高电平
- `PWM_SetCompare1()` 只是对 `TIM_SetCompare1(TIM2, x)` 的封装，方便 main 直接改占空比

## 学习点
- **PWM 为什么能做"呼吸"而不是闪烁**：1kHz 的周期是 1ms，远快于人眼分辨极限；改占空比 → 改 LED 的平均功率 → 视觉上就是亮度渐变
- 互补/主从之外，普通定时器输出 PWM 的四步：开时钟 → 配 GPIO 复用 → 配时基单元 → 配输出比较单元 + `TIM_Cmd`
- 预分频和自动重装为什么要 `- 1`：寄存器里的值 +1 才是实际分频系数 / 计数上限
- 渐变的两段循环写法：`i` 递增做渐亮（CCR 0→99），`100 - i` 做渐暗（CCR 100→1），两段首尾相接所以看不出跳变

## 已知边界（诚实标注）
- **渐变用 `Delay_ms(30)` 阻塞实现**，`while(1)` 里干不了别的事。真实项目里改占空比应该交给定时器中断或 RTOS 任务，否则主循环被占死——这正是阶段二要解决的问题
- `main.c` 调用了 `OLED_Init()` 但**没有任何 `OLED_Show*` 调用**，是从 4-1 例程带过来的残留初始化，对呼吸灯功能无影响
- 工程目录里还有 `Timer.c / Timer.h`（6-1 的遗留文件，Keil 工程里仍参与编译），但 `main.c` 未调用 `Timer_Init()`，所以**本仓库未纳入**，避免和 `PWM.c` 里对 TIM2 的配置混淆

## 文件
- `main.c` — 主程序（两个 for 循环做渐亮 / 渐暗）
- `PWM.c / PWM.h` — TIM2_CH1 PWM 输出驱动（初始化 + 改占空比）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
