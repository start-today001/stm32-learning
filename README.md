# STM32 Learning Journey

从零学习嵌入式 C 和 STM32 的完整记录。
始于 2026 年 7 月。

## 当前进度（2026-10-28）

| 阶段 | 内容 | 状态 |
|---|---|---|
| 阶段一 · 标准库 | 教程 3-1 ~ 9-4（GPIO / OLED / TIM / PWM / 捕获 / 编码器 / ADC / DMA / USART） | 21 个快照完成 |
| 阶段二 · HAL | CubeMX + HAL 库 | 2 个快照（LED / DMA+ADC 4 通道） |
| 在学 | I2C 协议 · 从机 MPU6050，补软件 I2C 的「读」 | 驱动编写中 |

详细断点、待办与中断记录见 **[PROGRESS.md](PROGRESS.md)**。

## 目录结构

```
├── phase1-stm32-basics/   ← STM32 基础外设（标准库：3-1 ~ 9-4）
│   ├── 01-gpio-led/       ← GPIO 点灯
│   └── …                  ← 共 21 个快照
├── phase2-hal/            ← HAL 库 + CubeMX（计划书 W11 起的新内容）
│   ├── 01-led-blink/      ← HAL 3-1 LED 闪烁（第一个 CubeMX 工程）
│   └── 02-dma-adc-4ch/    ← HAL 8-2 DMA + ADC 4 通道（扫描 + 循环 DMA）
└── phase2-can/            ← CAN 通信（后续）
```

## 约定

- 一个教程单元 = 一个快照目录，目录内 `README.md` 写硬件接线、功能、学习点与实测记录
- 快照在**代码跑通、实测过**之后才归档，不提交半成品
- 每个快照一个 `feat:` 提交
