# 8-1 DMA 数据转运（存储器 → 存储器）

## 硬件
- MCU: STM32F103C8T6
- OLED: SCL = PB8、SDA = PB9（软件模拟 I2C）
- 本实验不接外部器件，纯片内两段内存之间的搬运

## 功能
- 定义两个 4 字节数组：`DataA`（源）、`DataB`（目标）
- 用 **DMA1 通道 1** 以「存储器 → 存储器（M2M）」方式把 `DataA` 搬到 `DataB`
- OLED 上半部分显示 `DataA`（标签 + 地址 + 4 个字节），下半部分显示 `DataB`
- 主循环：每秒给 `DataA` 四个字节各 +1 → 显示 → 延时 1 秒 → **调用 DMA 转运** → 再显示

**这个循环的设计意图**：让人亲眼看到「**源数据变了，但 `DataB` 只有在 `MYDMA_Transfer()` 之后才跟着变**」—— 这就是 DMA 的"手动触发、硬件搬运"。

## DMA 配置要点（MYDMA.c）
```c
RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);   // ★ DMA 挂在 AHB，不是 APB

DMA_InitStructure.DMA_PeripheralBaseAddr = AddrA;         // 源
DMA_InitStructure.DMA_PeripheralDataSize = ..._Byte;
DMA_InitStructure.DMA_PeripheralInc      = ..._Enable;
DMA_InitStructure.DMA_MemoryBaseAddr     = AddrB;         // 目标
DMA_InitStructure.DMA_MemoryDataSize     = ..._Byte;
DMA_InitStructure.DMA_MemoryInc          = ..._Enable;
DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;   // 外设是源
DMA_InitStructure.DMA_BufferSize         = Size;                    // 4
DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
DMA_InitStructure.DMA_M2M                = DMA_M2M_Enable;          // ★ 存储器到存储器
DMA_InitStructure.DMA_Priority           = DMA_Priority_Medium;

DMA_Init(DMA1_Channel1, &DMA_InitStructure);
DMA_Cmd(DMA1_Channel1, DISABLE);        // 初始化完先不启动，等 Transfer 时再启
```

### ★ M2M 模式下的「角色借用」
DMA 硬件原本是为「**外设 ↔ 内存**」设计的，**没有"内存→内存"这种角色**。所以在 M2M 模式下必须**借用**：

| 字段 | 本项目填 | 含义 |
|---|---|---|
| `DMA_PeripheralBaseAddr` | `DataA`（源） | 把**源**当成"外设" |
| `DMA_MemoryBaseAddr` | `DataB`（目标） | 把**目标**当成"内存" |
| `DMA_DIR` | `DMA_DIR_PeripheralSRC` | 方向 = 外设是源 |

这三处必须**配对正确**，否则搬反或不动。

## 为什么 `MYDMA_Transfer()` 里要先重装载计数器
```c
void MYDMA_Transfer(void)
{
    DMA_Cmd(DMA1_Channel1, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel1, MYDMA_Size);   // ★ 重装载
    DMA_Cmd(DMA1_Channel1, ENABLE);
    while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);    // 等传输完成
    DMA_ClearFlag(DMA1_FLAG_TC1);
}
```
- `DMA_Mode_Normal`：传完 `BufferSize` 个数据后**自动停止**，且 `CNDTR` 减到 **0**
- 所以再次传输前**必须重设 `CNDTR`**，否则 `BufferSize = 0` → 一个字节都不会搬
- 这是很多初学者漏掉的一步

## 学习点

### 1. `const` 决定数组在 Flash 还是 RAM
```c
uint8_t DataA[] = {0x01, 0x02, 0x03, 0x04};   // 无 const → RAM 的 .data 段
uint8_t DataB[] = {0, 0, 0, 0};
```
- **加 `const`** → 编译器把数组放进 **Flash**（只读，不占 RAM）
- **不加 `const`** → 放进 **RAM**，启动时代码把初值从 Flash 拷到 RAM
- 本工程**不能加 `const`**，因为主循环要 `DataA[i]++` —— 所以这里是 **RAM → RAM** 转运
- 《江科大》原版用的是 `const uint8_t DataA[]`，演示的是 **Flash → RAM**。两种都合法，但**要知道区别在哪**：面试常问「`const` 变量存在哪」「为什么全局数组占 RAM 而常量字符串占 Flash」

### 2. 等 DMA 完成：忙等 vs 中断
```c
while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);   // 忙等
```
- 本项目搬 4 个字节，等几微秒，无所谓
- **真实项目里应该用中断**（`DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE)` + NVIC），否则 CPU 白等着 —— 而 **DMA 存在的意义恰恰是"让 CPU 去干别的事"**。用忙等去等 DMA，就把 DMA 的价值抵消了一半
- 这也是后面 `8-2 DMA + AD 多通道` 会展开的方向

### 3. 为什么用 `DMA1_Channel1`
M2M 模式与具体通道无关（不像外设请求会绑定固定通道），**任意通道都能用**。

## 已知边界（诚实标注）
- **模块文件名已修正**：原为 `system/MYDAM.c`（把 `DMA` 写成了 `DAM`，而 `.h` 叫 `MYDMA.h`、函数叫 `MYDMA_*`、保护宏是 `__MYDMA__H` —— 四个名字里就这一个错）。本仓库上传的是**修正后**的 `MYDMA.c`，Keil 工程 `prjoct.uvprojx` 里的两处引用（`FileName` / `FilePath`）也已同步更新
  - ⚠️ 工作区文件夹名 `8-1 DAM传输` 也含同一个错别字（未改）
- `main.c` 第 33~42 行保留了一段**被注释掉的重复代码**（8 行，写的是往第 3/4 行输出，属于早期版本残留，且第 3 行会与 `DataB` 标签撞车）—— **按"发现写进 README"的取舍保留未删**
- `MYDMA_Init()` 里 `DMA_Cmd(DISABLE)`、`MYDMA_Transfer()` 开头又 `DMA_Cmd(DISABLE)` —— 重复一次，无害
- `MYDMA.h` 使用了 `uint32_t` / `uint16_t` 但没有自己 include `stm32f10x.h`，依赖调用方先包含 —— 全项目同类头文件都这样，暂不改
- `MYDMA.h` 的保护宏是 `__MYDMA__H`（双下划线，C 标准保留给实现）
- `KEY.c/h`、`LED.c/h` 未被任何文件 include，未纳入

## 文件
- `main.c` — 主程序（数据变化 → 显示 → DMA 转运 → 再显示的演示循环）
- `MYDMA.c / MYDMA.h` — DMA 驱动（M2M 初始化 + 重装载并启动转运）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库；`ShowHexNum` 用来显示地址与字节）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
