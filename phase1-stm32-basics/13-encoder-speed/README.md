# 6-8 编码器接口测速

## 硬件
- MCU: STM32F103C8T6
- **旋转编码器 A/B 相**: PA6 / PA7（上拉输入 `GPIO_Mode_IPU`，即 TIM3_CH1 / TIM3_CH2）
- OLED: SCL = PB8、SDA = PB9（软件模拟 I2C）

## 功能
- TIM3 用**硬件编码器接口**解码旋转编码器的 A/B 两相
- TIM2 **每 1 秒**产生一次更新中断，在中断里读取这一秒的编码器增量
- OLED 第 1 行显示转速（带符号：正数为正转，负数为反转）

## 编码器接口原理
- 旋转编码器输出两路方波 A/B 相，**相位差 90°**，转动方向由相位差的超前关系决定：
  - A 相超前 B 相 → 正转
  - B 相超前 A 相 → 反转
- STM32 定时器内置**硬件编码器接口**：把 A/B 相接到 TI1/TI2 后，硬件根据两相的边沿组合自动对 `CNT` **+1 / −1 / 不变**
- **CPU 完全不用管脉冲计数** —— 只需定期读一次 `CNT`。这是定时器"测外部世界"三种用法里**最省 CPU** 的一种

## 配置要点（Encoder.c）
```c
TIM_TimeBaseInit:  ARR = 65536 - 1     // 16 位满量程
                   PSC = 1 - 1         // 不分频，最大化计数分辨率

TIM_ICInit(通道1, Filter = 0xF)        // ← 设输入滤波器
TIM_ICInit(通道2, Filter = 0xF)
TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, Rising, Rising)
TIM_Cmd(TIM3, ENABLE)
```

### 为什么 `TIM_ICInit` 和 `TIM_EncoderInterfaceConfig` 都要调？
这两个函数都会写 `CCMR1`，但**职责不同**（逐位核对过库源码 `stm32f10x_tim.c`）：

| 函数 | 负责的寄存器位 |
|---|---|
| `TIM_ICInit` | 通道映射（`CC1S`/`CC2S`）、**输入滤波器（`IC1F`/`IC2F`）**、极性 |
| `TIM_EncoderInterfaceConfig` | 编码器模式（`SMS`）、通道映射（`CC1S`/`CC2S`）、极性（`CC1P`/`CC2P`）—— **完全不碰滤波器** |

所以正确顺序是：**先用 `TIM_ICInit` 把滤波器设成 `0xF`，再用 `TIM_EncoderInterfaceConfig` 设编码器模式**（后者保留滤波器设置）。顺序反了，滤波器就丢了。

### `TIM_EncoderMode_TI12` 的含义
**TI1 和 TI2 两相的边沿都计数** → 分辨率是单相的 4 倍（常说的"4 倍频"）。也因为这个，算实际转速时记得除以 4。

## 测速方式（Timer.c + main.c）
```c
// Timer.c：TIM2 每 1 秒中断一次
PSC = 7200 - 1  → 10kHz
ARR = 10000 - 1 → 1Hz  （即 1 秒一次更新中断）

// main.c：在 TIM2 中断里读增量
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        Speed = Encoder_Get();
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
```
- `Encoder_Get()` = **读 `CNT` 然后清零** → 返回的就是"这 1 秒内转了多少个计数"，即**增量式测速**（对比：绝对式是读累计值）
- 反向转动时 `CNT` 递减、从 0 绕到 65535，赋给 `int16_t` 得到 `-1` ✅（依赖二进制补码）

## 学习点

### 1. `volatile` —— 本项目里最该注意的一行
```c
volatile int16_t Speed;
```
- `Speed` **在中断服务函数里被写**、**在 `while(1)` 里被读** → 这是**必须加 `volatile`** 的经典场景
- 不加会怎样：编译器看不到主循环里有修改 `Speed` 的路径，可能把它**缓存进寄存器**，于是显示永远不刷新（优化等级越高越明显）
- 这正是「ISR 与主循环共享变量」的标准答案，也是嵌入式面试高频题

### 2. 编码器接口 vs 输入捕获
| | 输入捕获（6-6 / 6-7） | 编码器接口（6-8） |
|---|---|---|
| 测什么 | 周期 / 占空比 | 脉冲个数（含方向） |
| 硬件做什么 | 在边沿**锁存** CNT | 直接对 CNT **加减** |
| CPU 参与度 | 读 CCR | 定期读 CNT |

### 3. 定时器"测外部世界"的三种方法 —— 本仓库的完整演进
1. **测周法**测频率 → `phase1-stm32-basics/11-input-capture`（6-6）
2. **PWMI** 测频率 + 占空比 → `12-pwmi-duty`（6-7）
3. **编码器接口**硬件解码计数 → 本工程（6-8）

## 已知边界（诚实标注）

- **溢出边界**：`Encoder_Get()` 返回 `int16_t`，上限 ±32767。若 1 秒内编码器脉冲超过 32767 个，数值会算错。手转编码器没问题，**接电机前要按「转速 × 线数 × 4」算清上限**，超出就得缩短采样周期
- **读-清不是原子操作**：`Temp = TIM_GetCounter(TIM3); TIM_SetCounter(TIM3, 0);` 这两步之间如果来了一个脉冲，那一个计数就丢了（此时中断是开着的）。**真实车控项目里读编码器要么用硬件锁存，要么进临界区** —— 这是可以往深里聊的点
- `Timer.c` 里原有的 `extern uint16_t Num;`（6-1 遗留，`Num` 在本工程**没有任何定义**）和一段被注释的 `TIM2_IRQHandler`（内含 6-1 那个 `TIM_IT_Update == SET` 括号错位 bug）**已删除** —— 后者若被取消注释会与 `main.c` 里的正确版本**重复定义**，导致链接错误
- `KEY.c/h`、`LED.c/h` 未被任何文件 include，未纳入
- `Timer.h` 的保护宏是 `__Timer_H`（混合大小写，与仓库其他文件的全大写风格不一致；且下划线开头属 C 标准保留标识符）—— 暂不改
- `Encoder.h` 里 `int16_t Encoder_Get(void);` 后多了两个空格和一个空行 —— 未清理
- `main.c` 保留了一行被注释的调试代码 `//OLED_ShowNum(2, 5, TIM_GetCounter(TIM2), 5);`

## 文件
- `main.c` — 主程序（初始化 + TIM2 中断服务函数读取转速 + OLED 显示）
- `Encoder.c / Encoder.h` — 编码器驱动（TIM3 编码器接口初始化 + 增量读取）
- `Timer.c / Timer.h` — TIM2 1 秒时基 + NVIC 中断配置
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
