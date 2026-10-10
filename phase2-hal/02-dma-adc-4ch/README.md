# HAL 8-2 DMA + ADC 4 通道（扫描 + 循环 DMA）

> 阶段二 HAL 主线第 2 个工程。教程对照：江协 HAL 重制版 **18. DMA+AD多通道**。
> 标准库版的 8-2 没做，直接按 HAL 口径补上（计划书：W11 起新内容用 HAL 库 + CubeMX）。

## 当前状态（2026-10-10）

- ✅ CubeMX 配置 + 生成 + Keil 编译 **0 Error / 0 Warning**（`Code=7264 RO-data=1848` ≈ 8.9 KB）
- ✅ **业务代码已落地**：`USER CODE PV` 定义 `adc_buf[4]` / `vol[4]` → `USER CODE 2` 里
  `HAL_ADCEx_Calibration_Start()` + `HAL_ADC_Start_DMA()` → `while(1)` 里换算 + 4 行显示
- ✅ OLED 已通（标准库驱动移植到 HAL，改动只有 3 处；`OLED OK` 点亮过）
- ⏳ **实机验收还没做**（当时手边没设备）—— 待验项见文末

## 硬件

- MCU: STM32F103C8T6
- **4 路模拟输入**: PA0 / PA1 / PA2 / PA3（= `ADC1_IN0~IN3`；CubeMX 会自动把它们设成模拟模式）
- OLED: SCL = PB8、SDA = PB9（软件 I2C，**开漏输出 + 内部上拉**）
- 时钟：HSE 8 MHz → PLL×9 → 72 MHz；APB1 36 / APB2 72；**ADC 预分频 ÷6 = 12 MHz**（硬约束 ≤14 MHz）

## 关键配置（已在生成代码里逐项核过）

- **ADC1**：`ScanConvMode = ADC_SCAN_ENABLE`、`ContinuousConvMode = ENABLE`、
  `NbrOfConversion = 4`、右对齐、`ADC_SOFTWARE_START` 软件触发、
  4 个 Rank 依次对应 `ADC_CHANNEL_0~3`，采样时间都是 `55CYCLES_5`
- **DMA**：`DMA1_Channel1`（ADC1 的硬件固定通道）、Peripheral → Memory、
  两边数据宽度都 **Half Word**、Memory 地址自增 / Peripheral 不自增、**Mode = Circular**
- `hdma_adc1` 的初始化写在 **`HAL_ADC_MspInit()`**（`Core/Src/stm32f1xx_hal_msp.c`）里，
  **不在** `MX_DMA_Init()` 里 —— F1 的 CubeMX 就是这个套路（`MX_DMA_Init` 只开时钟和 NVIC）
- 中断 `DMA1_Channel1_IRQHandler` 已生成并调用 `HAL_DMA_IRQHandler(&hdma_adc1)`

## 学习点 / 待写的代码

- 校准：`HAL_ADCEx_Calibration_Start(&hadc1)` —— **F1 版只有一个参数**（F4/H7 才有第二个）
- 启动：`HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 4)` —— **要强转**、**只调一次**（循环模式下它自己一直刷 buf）
- 换算：`vol[i] = adc_buf[i] / 4095.0f * 3.3f;`
- **Rank 顺序 = `adc_buf` 下标顺序**：Rank1→`buf[0]`、Rank2→`buf[1]`……
- 「多通道扫描必须靠 DMA」的根因：`ADC_DR` 只有一个，轮询会互相覆盖（这也是 7-2 的结论）

## 已知边界 / 待办

- ⏳ **实机验收待做**：4 路互相独立 / 短接 GND ≈ 0 / 短接 3.3 V ≈ 4095 / 有万用表就比对误差 < 0.05 V
- ⏳ 电压 `vol[]` 已算出但还没显示；本驱动的 `OLED.h` 里**没有 `OLED_ShowFloat`**，要显示 `1.650V`
  得先换成毫伏整数（`(uint16_t)(vol[i] * 1000)`）
- 排查过的三个坑（都值得记住）：
  1. **F1 HAL 里没有 `DMA Continuous Requests` 字段**（那是 F4/H7 的），别照着别的教程去找它
  2. **`adc_buf[1]` 下标写死编译器不报错** —— 现象是「四路读数完全一样」，改回 `adc_buf[i]`
  3. **用 `.ioc` 核查时，F1 ADC 的 `ScanConvMode` 不写进文件** —— 缺行 ≠ 默认值，这项只能看 UI 或生成后的代码

## 文件

- `DMA+ADC.ioc` — CubeMX 工程定义（**唯一权威**；`Drivers/`、`MDK-ARM/` 不入库）
- `Core/Src/main.c` — 生成的外设初始化 + 待补的业务逻辑（`USER CODE` 区）
- `Core/Src/stm32f1xx_hal_msp.c` — HAL 底层初始化（ADC 时钟、模拟引脚、DMA 句柄）
- `Core/Src/stm32f1xx_it.c` / `Core/Inc/stm32f1xx_it.h` — 中断入口
- `Core/Inc/main.h`、`Core/Inc/stm32f1xx_hal_conf.h`
