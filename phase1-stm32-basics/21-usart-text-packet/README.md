# 9-4 串口收发文本数据包

> 江科大 STM32 入门教程 · 第 9 章 USART 串口通信 · 第 4 集（**文本数据包 + 命令响应**）
> 和 9-3 同为"数据包"，但协议换成**人可读的文本**，并做成**收到命令就执行并回 ACK** 的闭环。

## 硬件
- MCU: STM32F103C8T6
- **串口 USART1**：TX = **PA9**（`GPIO_Mode_AF_PP`）、RX = **PA10**（`GPIO_Mode_IPU` 上拉输入）
- **LED**：**PA1 / PA2**（`LED_Init()` 配成推挽输出，**低电平点亮**）—— 本集收到命令后点亮/熄灭 LED1
- OLED: SCL = PB8、SDA = PB9（软件 I2C）
- USB 转 TTL 模块：模块 RX→PA9、模块 TX→PA10、**共地**

## 功能
- 串口 **9600 / 8N1**
- **文本数据包格式**：`@` + 若干可读字符 + `\r\n`

| 位置 | 内容 |
|---|---|
| 包头 | **`'@'`** |
| 数据 | **任意文本**（如 `Led_ON`） |
| 包尾 | **`'\r'` + `'\n'`**（回车换行） |

- **收到命令后执行并回响应**：

| 收到的文本包 | 动作 | 回给上位机的响应 |
|---|---|---|
| `@Led_ON\r\n` | `LED1_ON()` 点亮 LED | `LED_ON_OK\r\n` |
| `@Led_OFF\r\n` | `LED1_OFF()` 熄灭 LED | `LED_OFF_OK\r\n` |

- OLED 显示：第 3 行 `RxData:` 后跟收到的文本（第 9 列起）

## 学习点

### 1. 文本协议 vs HEX 协议（对应 9-3）
| | HEX 数据包（9-3） | 文本数据包（9-4） |
|---|---|---|
| 长相 | `FF 01 02 03 04 FE` | `@Led_ON\r\n` |
| 优点 | 紧凑、解析快、适合机器对机器 | **人可读**、串口助手里直接敲命令、便于调试 |
| 缺点 | 人看不懂，得换算 | 占字节多、要按字符串处理 |
| 结束判定 | 固定长度（4 字节）或包尾字节 | **靠 `\r`（回车）判定数据结束**、`\n` 收尾 |

### 2. 状态机拼字符串 + `'\0'` 结尾
```c
char Serial_RxPacket[100];      /* 收满 100 字节的文本 */

switch (RxState)
{
    case 0:                                   /* 找包头 '@' */
        if (RxData == '@') { RxState = 1; pRxState = 0; }
        break;
    case 1:                                   /* 收文本，直到 '\r' */
        if (RxData == '\r') { RxState = 2; }  /* 遇到回车 → 转去找 '\n' */
        else { Serial_RxPacket[pRxState] = RxData; pRxState++; }
        break;
    case 2:                                   /* 找 '\n'，然后收尾 */
        if (RxData == '\n')                   /* ← 判的是"数据"字节，不是下标 */
        {
            RxState = 0;
            Serial_RxPacket[pRxState] = '\0'; /* 关键：补字符串结束符 */
            Serial_Rxflag = 1;
        }
        break;
}
```
- **`'\0'` 必须补**：C 里所有字符串函数（`strcmp`/`printf("%s")`…）都靠 `'\0'` 判断"到哪结束"；不补就会一直往后读内存
- **`@` 只表示"包头"**，不进数据区；`pRxState = 0` 是在检测到包头时复位的（保证每包从 `RxPacket[0]` 开始写）

### 3. 命令-响应模式（工程里最常见的交互）
```c
if (Serial_GetRxFlag() == 1)
{
    OLED_ShowString(3, 9, Serial_RxPacket);          /* 先显示收到的内容 */
    
    if      (strcmp(Serial_RxPacket, "Led_ON")  == 0) { LED1_ON();  Serial_SendString("LED_ON_OK\r\n");  }
    else if (strcmp(Serial_RxPacket, "Led_OFF") == 0) { LED1_OFF(); Serial_SendString("LED_OFF_OK\r\n"); }
}
```
- **`strcmp(a, b) == 0` 才是"相等"**（返回 0 表示相同，不是返回 1）
- 上位机（串口助手）敲 `@Led_ON` + 回车 → 板子亮灯并回 `LED_ON_OK`，形成**闭环验证**
- **回 ACK 的意义**：上位机知道"命令到了、执行了"，比"只发不等回"可靠得多

### 4. 接收三件事 + 中断顺序（与 9-2/9-3 相同，但这次有坑）
```c
GPIO_Mode_IPU                       /* ① RX 脚上拉输入 */
USART_Mode_Tx | USART_Mode_Rx       /* ② Mode 必须含 Rx（RE 位）*/
USART_Cmd(USART1, ENABLE);          /* ③ 使能 */

if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)   /* ④ 先判标志 */
    uint8_t RxData = USART_ReceiveData(USART1);        /*    再读 DR（读 DR 清 RXNE）*/
```
- `USART_ReceiveData()` 是**读 `DR`**，**读一次 `RXNE` 就被硬件清掉** → 顺序反了 `GetITStatus` 就恒为 `RESET`，字节全丢

### 5. `LED1_ON()` / `LED1_OFF()` 依赖 `LED_Init()`
```c
LED_Init();          /* 把 PA1/PA2 配成 Out_PP 并把两个灯都熄灭 */
...
LED1_ON();           /* 只是 GPIO_ResetBits(GPIOA, GPIO_Pin_1) —— 写 ODR */
```
**不调 `LED_Init()` 会怎样**：PA1 保持复位后的**浮空输入**状态，`GPIO_ResetBits` 只是写了输出寄存器，引脚没有推挽输出能力 → **灯不亮**。

## 调试记录（本工程上传前修改了 5 处）

同样用"假 USART"在 PC 上编译并跑了**真实的 `Serial.c`** 验证。

| # | 问题 | 后果 | 修法 |
|---|---|---|---|
| **1** | `USART_ReceiveData()` 写在 `GetITStatus()` **之前**（在 `if` 外面） | **每个字节都被丢弃**（PC 实测：`Rxflag` 保持 0、`RxPacket` 为空字符串） | 挪进 `if` 里、判断放前面 |
| **2** | `case 2` 里写成 `if (pRxState == '\n')`（**数据写成了下标**） | 收 `@Led_ON\r` 后 `pRxState=6`，判 `6 == 10` 不成立 → **状态机永远卡在 `case 2`** → 命令永远不执行、标志位永不置起 | 改成 `if (RxData == '\n')` |
| **3** | 调用了 `LED1_ON()/LED1_OFF()` 但**从没调 `LED_Init()`** | PA1 没配成推挽输出 → **灯根本不亮**（本集核心功能失效） | 补 `LED_Init();` |
| **4** | 残留 `Serial_SendPacket()`：从 9-3 抄来后删掉了 `Serial_TxPacket`，函数体只剩 `Serial_SendByte(0xFF); Serial_SendByte(0xFE);` | 这是 **HEX 协议的包头/包尾**，在文本协议里毫无意义，而且**从没被调用**（死代码），却在 `Serial.h` 里还声明着 | 删除函数与声明 |
| **5** | OLED 显示写在第 1 行、清行用 **21 个空格** | 收到的文本写第 1 行会**覆盖 `"TxData:"` 标签**，而 `"RxData:"`（第 3 行）永远空白；OLED 一行只有 16 格，21 个空格会**溢出到第 2 行前 5 格** | 改到**第 3 行第 9 列**起显示；清行改成 8 个空格（正好清第 9~16 格） |

另外顺手清理：删掉没用到的 `uint8_t KeyNum;` 和 `#include "kEY.h"`（本工程完全不需要 KEY）。
（注意 `kEY.h` 那个**大小写**问题在 9-3 里是真隐患，这里因为是删掉整行所以一并消失。）

**修复后 PC 实测（真实 `Serial.c`）**：**7 项全 PASS** ——
`@Led_ON\r\n`/`@Led_OFF\r\n`/`@ABC\r\n` **连收三包**内容都正确（同时验证 `pRxState` 每包都复位）、`Serial_GetRxFlag()` 读后清零、`Serial_SendString("LED_ON_OK\r\n")` 发出 11 字节正确。

## 已知边界（诚实标注）

- **⚠️ `Serial_RxPacket[100]` 没有上界检查**：`case 1` 里 `pRxState++` 不受限制 —— 如果收到 `@` 之后**一直不给 `\r`**，写入会一路越过 100 字节，**越界踩内存**（本工程固定的短命令没问题，但这是安全隐患）。建议加 `if (pRxState < sizeof(Serial_RxPacket) - 1)` 保护
- **第 3 行只有 8 格可用**（第 9~16 列）：收到的文本若超过 8 个字符，会继续写到第 4 行开头。本集的 `Led_ON`/`Led_OFF` 都在 8 字符内，不会越界
- **第 1 行那个 `"TxData:"` 标签是 9-3 留下的**：本集没有"发送数据包"的概念（只回 ACK 字符串），所以第 1 行标签下面**永远没有数据**。保留原样以便对照 9-3 的差异
- **`Serial_Rxflag` / `Serial_RxPacket` 严格说应加 `volatile`**：中断里写、主循环读。本工程通过 `Serial_GetRxFlag()`（另一个 .c 里的函数）访问，编译器无法假定它不改全局，实际安全；**同一文件内直接读且开了优化时**就可能被缓存 —— 进阶必知
- 继承 9-1/9-2/9-3 的老问题：`Serial_SendArray` 的 `length`/`i` 类型不匹配；`Serial_Pow` 未 `static` + 无符号次数陷阱；`__SERIAL__H` 保护宏；`Serial.h` 导出 `USART1_IRQHandler` 原型；`NVIC_PriorityGroupConfig()` 放在 `Serial_Init()` 里
- `KEY.c/h` 未被本工程引用，未纳入（对比 9-3 是**用了 KEY、不用 LED**）；Keil 的 `Objects/` `Listings/` `DebugConfig/` `EventRecorderStub.scvd` 等产物未纳入

## 文件
- `main.c` — 主程序（初始化 + 收到文本包后匹配命令 → 点灯 + 回 ACK + 显示）
- **`Serial.c / Serial.h`** — ★ 核心：串口收发 + **文本状态机** + `Serial_SendString`（回响应）
- **`LED.c / LED.h`** — LED 驱动（PA1/PA2 推挽输出，低电平点亮；与 3-x 的驱动相同）
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供）
- `Delay.c / Delay.h` — 软件延时
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（真正的中断服务函数在 `Serial.c` 里，`it.c` 里那处是注释态，不会重复定义）
