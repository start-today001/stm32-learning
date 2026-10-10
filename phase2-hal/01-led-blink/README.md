# HAL 3-1 LED闪烁（第一个 CubeMX 工程）

> **阶段二的起点**：从这一篇开始，新内容按计划书口径走 **HAL 库 + CubeMX**（W11 起）。
> 教程对照：江协 HAL 重制版 **3-1**。

## 当前状态（2026-10-10）

- ✅ CubeMX 6.17.0 配置并生成 MDK-ARM 工程；Keil 编译 **0 Error / 0 Warning**
- ⚠️ **闪灯逻辑还没写**：`Src/main.c` 的 `USER CODE BEGIN 3` 是空的 —— 也就是说当前这版**烧进去灯不会动**。
  本工程此刻的价值是「把 CubeMX → Keil 这条链走通」，代码待补。

## 硬件

- MCU: STM32F103C8T6（LQFP48）
- LED: **PA0**（推挽输出，低电平点亮）
- 时钟：**保持默认 HSI 8 MHz**（第一轮故意不动时钟树，少一个变量）

## 学习点

- CubeMX 的图形配置 ↔ 生成的 `MX_GPIO_Init()` 一一对应：
  `__HAL_RCC_GPIOA_CLK_ENABLE()`（时钟）→ `HAL_GPIO_WritePin(..., GPIO_PIN_RESET)`（初值）→
  `GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP`（模式）—— 和标准库三件套是同一件事的两种写法
- **`SYS → Debug` 必须选 `Serial Wire`**：CubeMX 新建工程默认是 `No Debug`，保持默认生成出来的工程 **SWD 被关**，ST-Link 烧不进程序
- **Toolchain 选 `MDK-ARM` + Min Version `V5`**：Min Version 选高版本会按 AC6(armclang) 生成，Keil 5.24 的 ARMCC(AC5) 编不过
- 代码只能写在 `/* USER CODE BEGIN */ … /* USER CODE END */` 之间，否则重新 Generate 会被冲掉

## 已知边界

- 闪灯代码待补（`HAL_GPIO_TogglePin()` + `HAL_Delay()`）
- 时钟树还没配（跑在 HSI 8 MHz），HSE 8 MHz + PLL×9 = 72 MHz 留到 02 工程
- 尚未实机烧录验证

## 文件

- `LED_Blink.ioc` — CubeMX 工程定义（**唯一权威**，用它可重建整个工程；`Drivers/`、`MDK-ARM/` 不入库）
- `Src/main.c` / `Inc/main.h` — 主程序与头文件（业务代码写在 `USER CODE BEGIN 3`）
- `Src/stm32f1xx_hal_msp.c` — HAL 外设底层初始化（时钟 / GPIO）
- `Src/stm32f1xx_it.c` / `Inc/stm32f1xx_it.h` — 中断入口
- `Inc/stm32f1xx_hal_conf.h` — HAL 模块裁剪配置

> 注：本工程的目录布局是 `Src/` + `Inc/`；同期的 `02-dma-adc-4ch` 是 `Core/Src` + `Core/Inc` ——
> CubeMX 生成的两份工程实测布局不同，以各自 `.ioc` 重新生成后的实际结构为准。
