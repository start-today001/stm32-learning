# 9-3 串口收发 HEX 数据包

> 江科大 STM32 入门教程 · 第 9 章 USART 串口通信 · 第 3 集（**自定义 HEX 数据包**）
> 在 9-2 的基础上把收发升级成"数据包"：用**包头/包尾**界定一个包，中断里用**状态机**拼包。

## 硬件
- MCU: STM32F103C8T6
- **串口 USART1**：TX = **PA9**（`GPIO_Mode_AF_PP`）、RX = **PA10**（`GPIO_Mode_IPU` 上拉输入）
- **按键 KEY**：**PB1 / PB11**（`Key_Init()` 配成上拉输入）—— 本集用 K1 触发发包
- OLED: SCL = PB8、SDA = PB9（软件 I2C）
- USB 转 TTL 模块：模块 RX→PA9、模块 TX→PA10、**共地**

## 功能
- 串口 **9600 / 8N1**
- **自定义数据包格式**（一个包共 6 字节）：

| 位置 | 内容 |
|---|---|
| 第 1 字节 | **包头 `0xFF`** |
| 第 2~5 字节 | **4 个数据字节** |
| 第 6 字节 | **包尾 `0xFE`** |

- **发送**：上电先发一包 `FF 01 02 03 04 FE`；之后每按一次 **K1**，4 个数据字节各 +1 再发一包
- **接收**：中断里用状态机拼包，收到一个合法包后置标志位，主循环把它显示到 OLED 第 3 行
- OLED 显示：第 1 行 `TxData:` 后跟 4 个发送字节的十六进制；第 3 行 `RxData:` 后跟 4 个接收字节

## 学习点

### 1. 为什么要"数据包"：单字节流没有边界
9-2 只能一个字节一个字节地收。**实际通信里一条消息往往好几个字节**（一条指令、一组传感器数据…），
接收方怎么知道"从哪开始、到哪结束"？——于是约定 **包头 + 数据 + 包尾**，这就是最简单的**数据包协议**。
本集用 `0xFF` 包头、`0xFE` 包尾（挑这两个值是因为它们在数据里不容易出现）。

### 2. 用**状态机**在中断里拼包（本集的核心）
```c
void USART1_IRQHandler(void)
{
    static uint8_t RxState  = 0;      /* 当前状态：0 找包头 / 1 收数据 / 2 找包尾 */
    static uint8_t pRxState = 0;      /* 已经收了几个数据字节 */
    
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)     /* ① 先判断标志位 */
    {
        uint8_t RxData = USART_ReceiveData(USART1);           /* ② 再读 DR（读 DR 会清 RXNE）*/
        switch (RxState)
        {
            case 0:                                   /* 找包头 */
                if (RxData == 0xFF)
                {
                    RxState  = 1;                     /* → 转入"收数据" */
                    pRxState = 0;                     /* 关键：包内计数清零 */
                }
                break;
            case 1:                                   /* 收 4 个数据字节 */
                Serial_RxPacket[pRxState] = RxData;
                pRxState++;
                if (pRxState >= 4) { RxState = 2; }   /* 够了 → 转去找包尾 */
                break;
            case 2:                                   /* 找包尾 */
                if (RxData == 0xFE)
                {
                    RxState = 0;                      /* 复位，准备下一包 */
                    Serial_Rxflag = 1;                /* 通知主循环"包好了" */
                }
                break;
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
```
三个要点，缺一个就收不到包：
1. **状态变量 `RxState` 要真的被改**（不是 `RxData`！）
2. **检测到包头时 `pRxState = 0`** —— 否则第 2 包起下标不复位，`Serial_RxPacket[pRxState]` 越界
3. **中断里先判标志、再读 `DR`**（见下一条）

### 3. ⚠️ 中断服务函数里的顺序：**先判断标志位，再读 `DR`**
```c
if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)   /* 必须在前面 */
{
    uint8_t RxData = USART_ReceiveData(USART1);        /* 必须在里面 */
```
**为什么不能反过来**（本工程原先就是反的）：
- `USART_ReceiveData()` 就是**读 `DR` 寄存器**
- F1 参考手册：`RXNE` 是 **"cleared by a read to the USART_DR register"** —— **读 `DR` 硬件立刻清 `RXNE`**
- 而 `USART_GetITStatus()` 的实现是「**中断使能位 `CR1` != 0 且 状态位 `SR` != 0** 才返回 `SET`」
- 所以先读 `DR` → `RXNE` 已被清 → `GetITStatus` 返回 `RESET` → **整个 `if` 不成立 → 字节被丢进局部变量后直接返回 → 一个包都收不到**

> 顺带记住：`USART_GetITStatus()`（看"使能位 && 标志位"）用在**中断里**；
> `USART_GetFlagStatus()`（只看标志位）用在**主循环查询**里。两者不能混着理解。

### 4. 数据包在两个文件之间共享：`extern`
```c
/* Serial.c 里"定义"（分配空间） */
uint8_t Serial_TxPacket[4];
uint8_t Serial_RxPacket[4];

/* Serial.h 里"声明"（告诉别的文件有这两个数组） */
extern uint8_t Serial_TxPacket[];
extern uint8_t Serial_RxPacket[];
```
- **定义只能有一份**（放 `.c`），**声明可以有很多份**（放 `.h`）
- 主循环想让"发什么"由自己决定，就**直接写 `Serial_TxPacket[...]`**，再调 `Serial_SendPacket()` 发出去

### 5. 接收必须三件事齐全（复习 9-2）
```c
GPIO_Mode_IPU          /* ① RX 脚配成上拉输入（不是复用输出）*/
USART_Mode_Tx | USART_Mode_Rx   /* ② Mode 里必须含 Rx，否则 CR1 的 RE=0 → 收不到 */
USART_Cmd(USART1, ENABLE);      /* ③ 使能 USART */
```

### 6. 发送侧照样要"封装"
```c
void Serial_SendPacket(void)
{
    Serial_SendByte(0xFF);                    /* 包头 */
    Serial_SendArray(Serial_TxPacket, 4);     /* 数据 */
    Serial_SendByte(0xFE);                    /* 包尾 */
}
```
**收发用同一套格式**，这样两台设备/两个程序才能互相看懂。

## 调试记录（本工程上传前修改了 6 处）

用一台"假 USART"（模拟"读 DR 清 RXNE"的硬件语义）在 PC 上编译并跑了**真实的 `Serial.c`** 来验证，修复前后对比：

| # | 问题 | 后果 | 修法 |
|---|---|---|---|
| **1** | `USART_ReceiveData()` 写在 `GetITStatus()` **之前**（在 `if` 外面） | **每个字节都被丢弃**，一个包都收不到（PC 实测：`RxPacket` 保持初值 `EE EE EE EE`） | 把读 `DR` 挪进 `if` 里、判断放前面 |
| **2** | `case 0` 里写成 `RxData = 1;`（**状态变量写成了数据变量**） | `RxState` 永远停在 0 → `case 1/2` **永不执行** → 接收完全无效 | 改成 `RxState = 1;` |
| **3** | 检测到包头时**没有复位 `pRxState`** | 第 2 包起 `Serial_RxPacket[pRxState]` **越界写**（数组只有 4 字节），踩坏相邻的 `Serial_Rxflag` | 补 `pRxState = 0;` |
| **4** | `#include "kEY.h"` 大小写与磁盘 `KEY.h` 不符 | Windows 能编，**Linux/macOS 编译失败** | 改成 `"KEY.h"` |
| **5** | 调用了 `Key_GetNum()` 但**从没调 `Key_Init()`** | KEY 引脚（PB1/PB11）没配成上拉输入 → 浮空输入读噪声 → **按键不可靠** | 补 `Key_Init();` |
| **6** | OLED 四组数据都写**第 1 行、从第 1 列起** | 把 `"TxData:"` 标签覆盖掉；接收数据也写第 1 行 → **收发互相覆盖** | 发送写第 1 行第 9/11/13/15 列；接收写**第 3 行**同列 |

**修复后 PC 实测（真实 `Serial.c`）**：**9 项全 PASS** ——
连收两包内容正确、`Serial_GetRxFlag()` 读后清零、截断包之后能恢复、`Serial_SendPacket()` 发出的帧是 `FF 01 02 03 04 FE`。

## 已知边界（诚实标注）

- **⚠️ 断包/丢字节后不能自动"重新同步"**（这套协议设计的固有局限，不是 bug）：
  状态机在"收数据"阶段**无法知道上一包是截断的**。实测：先发半包 `FF 55 66`，再发完整包 `FF A1 A2 A3 A4 FE`，结果是
  `RxPacket = 55 66 FF A1`（把下一个 `FF` 当成数据了），要等到那个 `FE` 才收尾复位，之后再发才恢复正常。
  **根因**：协议里**没有长度字段、没有超时机制**，只能靠"包头+包尾"对齐。更健壮的协议会加**长度字节**或**超时重同步**
- **`Serial_Rxflag` 严格说应该加 `volatile`**：它在中断里被写、在主循环里被读。本工程通过 `Serial_GetRxFlag()`（在另一个 .c 里，编译器无法假定它不改全局）访问，实际不会出错；但**同一个文件里直接读这类变量、又开了 `-O2` 时，可能被缓存进寄存器** —— 这是进阶必须知道的坑
- **OLED 一行只有 16 格**：4 个字节 × 2 位十六进制 = 8 格，正好填满第 9~16 格
- 继承 9-1/9-2 的老问题：`Serial_SendArray` 的 `length`(`uint32_t`) 与循环变量 `i`(`uint16_t`) 类型不匹配（`length > 65535` 会死循环）；`Serial_Pow` 未 `static` 且无符号次数陷阱；`Serial.h` 的保护宏 `__SERIAL__H` 用了保留标识符；`Serial.h` 里导出了 `USART1_IRQHandler` 原型（不必要）；`NVIC_PriorityGroupConfig()` 放在 `Serial_Init()` 里（全局设置，建议挪到 `main()` 只设一次）
- `LED.c/h` 未被本工程引用，未纳入；Keil 的 `Objects/` `Listings/` `DebugConfig/` `EventRecorderStub.scvd` 等产物未纳入

## 文件
- `main.c` — 主程序（初始化 + 按键触发发包 + 显示收发数据）
- **`Serial.c / Serial.h`** — ★ 核心：串口收发 + **状态机拼包** + `Serial_SendPacket`
- **`KEY.c / KEY.h`** — 按键驱动（PB1/PB11 上拉输入，`Key_GetNum` 返回键号）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供）
- `Delay.c / Delay.h` — 软件延时（KEY 消抖用）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（真正的中断服务函数在 `Serial.c` 里，`it.c` 里那处是注释态，不会重复定义）
