# 7-1 AD 单通道（ADC 采集 + 电压换算）

## 硬件
- MCU: STM32F103C8T6
- 模拟输入: PA0（`GPIO_Mode_AIN` 模拟输入，对应 **ADC1_IN0**）
- OLED: SCL = PB8、SDA = PB9（软件模拟 I2C）
- 被测对象：往 PA0 接模拟电压（电位器 / 传感器输出）

## 功能
- 轮询读取 ADC1 通道 0 的采样值（0~4095）
- 换算成电压（0~3.3V）显示
- OLED 第 1 行 `ADValue:xxxx`，第 2 行 `Voltage:x.xxV`

## ADC 配置要点（AD.c）
- **ADC 挂在 APB2**：`RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE)`（不是 APB1，容易记错）
- **ADC 时钟分频**：`RCC_ADCCLKConfig(RCC_PCLK2_Div6)` → 72MHz ÷ 6 = **12MHz**，满足 **ADC 时钟不得超过 14MHz** 的硬约束（F103 数据手册）
- 引脚用 `GPIO_Mode_AIN`（模拟输入）：数字输入/输出通路全部关闭，避免数字电路干扰模拟量
- `ADC_Mode_Independent`：独立模式（只用 ADC1）
- `ADC_ContinuousConvMode = DISABLE`：单次转换（软件触发一次转换一次）
- `ADC_DataAlign_Right`：12 位结果右对齐 → 读出来就是 0~4095
- `ADC_ScanConvMode = DISABLE`：非扫描模式（单通道）
- `ADC_ExternalTrigConv_None`：不用外部触发，靠软件启动
- `ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5)`：通道号 + 在规则序列里的次序 + 采样时间
- **校准流程（不能省）**：`ADC_Cmd(ENABLE)` → `ADC_ResetCalibration` 等完成 → `ADC_StartCalibration` 等完成。漏掉这步，采样值会有固定偏差

## 采样一次的过程（AD_GetValue）
```c
ADC_SoftwareStartConvCmd(ADC1, ENABLE);                    // 软件启动一次转换
while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);    // 等转换结束
return ADC_GetConversionValue(ADC1);                       // 读结果
```
- 采样时间 = `55.5 + 12.5 = 68` 个 ADC 时钟周期 ≈ **5.7µs**（12MHz 下）
- **为什么不需要显式清 EOC 标志**：`ADC_GetConversionValue()` 的源码就是 `return ADCx->DR;`，而 **读 `ADC_DR` 会自动清 EOC**（RM0008 规定）—— 所以这里不多写也不漏写

## 电压换算
```c
Voltage = (float)ADValue / 4095 * 3.3;   // 12 位 ADC，参考电压 3.3V
```
- 满量程是 **4095**（不是 4096）
- 显示必须拆成两次 `OLED_ShowNum`，因为模板 `"Voltage:0.00V"` 给整数位和小数位留了固定列位：
```c
OLED_ShowNum(2, 9,  (uint16_t)Voltage, 1);                     // 整数位 → col9
OLED_ShowNum(2, 11, (uint16_t)(Voltage * 100) % 100, 2);       // 两位小数 → col11~col12
```

## 学习点

### 1. `OLED_ShowNum` 是「从指定列连续写 N 个字符」，位数必须和模板对齐
这是本工程踩到的坑。原代码只有一行：
```c
OLED_ShowNum(2, 9, (uint16_t)(Voltage * 100) % 100, 2);   // 写 col9 ~ col10
```
而模板 `"Voltage:0.00V"` 的列分布是：

```
col1=V col2=o col3=l col4=t col5=a col6=g col7=e col8=: col9=0 col10=. col11=0 col12=0 col13=V
                                                                  ↑ col10 是小数点
```
`ShowNum(2, 9, …, 2)` 写 col9~col10 → **小数点被数字覆盖**，而且**整数位从来没被显示过**，实际显示成 `Voltage:2300V` 这种。

**规律：先数清模板每一列是什么，再决定每个 `ShowNum` 的起始列和位数。**

### 2. 模拟输入为什么要用 AIN 模式
数字输入带施密特触发器、上拉/下拉，会对模拟信号造成干扰并产生额外电流；AIN 模式把它们全部断开。

### 3. 校准为什么必须在采样前做
ADC 内部电容存在工艺偏差，校准过程把偏差记录成补偿值。不校准 → 结果有固定偏差，且偏差随芯片而变。

## 已知边界（诚实标注）
- **`volatile uint16_t ADValue;` 在本工程是多余的**：`ADValue` 只在 `while(1)` 里被写、被读，**没有任何中断参与**（本工程是轮询采样）。
  `volatile` 真正需要的场景只有三种：① ISR 与主循环共享的变量（如 `13-encoder-speed` 的 `Speed`）② 硬件寄存器 ③ 多线程/多核共享。加上无害，但被问「这个变量为什么要 volatile」时要能答得上来。
- **电压显示用的是「截断」而不是「四舍五入」**：`(uint16_t)(Voltage * 100) % 100` 先乘 100 再截断。
  实测遍历全部 4096 个 AD 值：约 **49.8%** 的值，小数末位比四舍五入**小 1**（最大偏差 0.01V）。
  - 想改成四舍五入：`(uint16_t)(Voltage * 100 + 0.5) % 100`
  - ⚠️ **顺带排除一个已被实测否掉的怀疑**：`float32` 的精度**没有**引入额外误差 —— 以精确整数运算 `ADValue × 330 / 4095` 为参照，4096 个值**逐个一致（0 个不符）**。所以这里的偏差纯粹来自「截断 vs 四舍五入」，**与浮点精度无关**。
- **`float` 其实可以避免**：显示电压不需要浮点，用整数 mV 即可，能省掉浮点库（flash + 速度）：
  `uint32_t Voltage_mV = (uint32_t)ADValue * 3300 / 4095;`（`4095 × 3300 = 1351 万`，`uint32_t` 不会溢出）
  保留 `float` 也能正常跑，本工程未改。
- `AD.h` 的保护宏是 `__AD__H`（双下划线，C 标准保留给实现）—— 全项目写法一致，暂不改
- `GPIO_InitStructure.GPIO_Speed` 在 `GPIO_Mode_AIN` 下**不起作用**（速度只影响输出驱动能力）—— 教程原版也这么写，无害
- `KEY.c/h`、`LED.c/h` 未被任何文件 include，未纳入

## 文件
- `main.c` — 主程序（初始化 + 循环采样、换算、显示）
- `AD.c / AD.h` — ADC 驱动（ADC1 初始化 + 校准 + 单次采样读取）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
