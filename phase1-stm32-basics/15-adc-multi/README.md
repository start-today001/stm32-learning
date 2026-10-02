# 7-2 AD 多通道（单次转换 + 非扫描模式）

> 工程文件夹名是 `7-1 AD多通道`，但课程编号实际是 **7-2**（接线图目录里有 `7-2 AD多通道.jpg` 佐证）。

## 硬件
- MCU: STM32F103C8T6
- 模拟输入: PA0 / PA1 / PA2 / PA3（`GPIO_Mode_AIN` 模拟输入，对应 **ADC1_IN0 ~ IN3**）
- OLED: SCL = PB8、SDA = PB9（软件模拟 I2C）

## 功能
- 轮询依次采集 ADC1 的 4 个通道（每个 0~4095）
- OLED 四行分别显示 `AD0:` ~ `AD3:` 加 4 位数值

## 多通道的实现思路（本项目走的路线）
```c
// AD.c
ADC_InitStructure.ADC_ScanConvMode = DISABLE;   // 非扫描模式
ADC_InitStructure.ADC_NbrOfChannel = 1;

uint16_t AD_GetValue(uint8_t ADC_Channel)
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_55Cycles5);  // 采样前换通道
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}
```
**核心动作**：每次采样前用 `ADC_RegularChannelConfig()` 重新指定通道 —— 这就是「单次转换 + 非扫描模式」实现多通道的做法。

## 两条实现路线的对比（本仓库的演进）
| | **本项目（7-2）** | **8-2「DMA + AD 多通道」** |
|---|---|---|
| 配置 | 单次转换 + **非扫描** | 连续转换 + **扫描** + DMA |
| 换通道 | **软件**每次调用 `ADC_RegularChannelConfig` | **硬件**按规则序列自动轮询 |
| CPU 参与 | 每次都要介入（启动 → 等 EOC → 读） | 启动一次，DMA 自动把结果搬到内存 |
| 代价 | ① 每个通道都要重配 ② 切通道后的采样受上一通道残留电荷影响 | 基本没有 |

⚠️ 关于第 ② 点要说准确：ADC 的**采样保持电容**在切换通道后会残留上一个通道的电荷。本项目采样时间是 **55.5 周期（够长）**，影响被大幅摊薄；但如果把采样时间调得很短，切通道后的第一次结果会明显偏向上一通道 —— **这正是 8-2 要考虑的问题之一**。

## 学习点

### 1. `OLED_ShowString(行, 列, "...")` / `OLED_ShowNum(行, 列, ..., 位数)` —— 行号和列号必须逐个对齐
本工程踩到的坑：四行标签**全写在「第 1 行第 1 列」**，后写的覆盖先写的 → 屏幕上只看到最后一个 `AD3:`，第 2~4 行完全没有标签；而数值却写在第 1~4 行 → **标签和通道完全对不上**。

**规律**：画界面时先把「第几行显示什么、每样东西占哪几列」列出来，再逐行写。
本例的正确分配：标签 `"AD0:"` 占 **col1~col4**，数值 `ShowNum(col5, 4位)` 占 **col5~col8** → 不重叠，一行显示成 `AD0:1234`。

### 2. 为什么每次都要重新配通道
因为这里是**非扫描模式**：ADC 只转换「规则序列里的第 1 个通道」，序列不会自动推进。改成**扫描模式**后 ADC 会按序列自动轮询多个通道，但结果都往同一个 `DR` 寄存器里塞、会互相覆盖 —— 所以必须配 **DMA** 才能把全部结果搬走（→ 8-2）。

### 3. 采样时间的选择
`ADC_SampleTime_55Cycles5` 在 12MHz 下约 5.7µs（含 12.5 周期的转换时间）。采样时间越长：抗干扰越好、对信号源阻抗要求越低，但总转换率越慢。

## 已知边界（诚实标注）
- **`volatile uint16_t AD0, AD1, AD2, AD3;` 在本工程是多余的** —— 这四个变量只在 `while(1)` 里被写、被读，**没有任何中断参与**。`volatile` 真正需要的场景只有三种：① ISR 与主循环共享的变量（如 `13-encoder-speed` 的 `Speed`）② 硬件寄存器 ③ 多线程/多核共享。（与 `14-adc-single` 相同的取舍：保留不改）
- **通道切换的残留电荷**：见「学习点」第 2 条的 ⚠️ 说明。本项目采样时间足够长，影响很小
- **工程文件夹名与课程编号不一致**：文件夹叫 `7-1 AD多通道`，课程编号是 **7-2**。仓库内目录名用 `15-adc-multi`，按学习顺序编号，不受影响
- `AD.h` 的保护宏是 `__AD__H`（双下划线，C 标准保留给实现）—— 全项目写法一致，暂不改
- `GPIO_InitStructure.GPIO_Speed` 在 `GPIO_Mode_AIN` 下**不起作用**（速度只影响输出驱动能力）
- `KEY.c/h`、`LED.c/h` 未被任何文件 include，未纳入

## 文件
- `main.c` — 主程序（4 个通道循环采样 + 四行显示）
- `AD.c / AD.h` — 4 通道 ADC 驱动（`AD_GetValue(通道)` 每次采样前重配通道）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
