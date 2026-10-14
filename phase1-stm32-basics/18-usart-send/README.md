# 9-1 串口发送

> 江科大 STM32 入门教程 · 第 9 章 USART 串口通信 · 第 1 集（发送部分）
> 本工程只做**发送**（`USART_Mode_Tx`）；接收在 9-2（`19-usart-rx`）补上。
>
> **后续补充（2026-10-14）**：加上 `fputc` 重定向，可以用 `printf` 打印 —— 见「学习点 6」。

## 硬件
- MCU: STM32F103C8T6
- **串口**: USART1
  - **TX = PA9**（`GPIO_Mode_AF_PP`，**复用推挽**输出 50MHz）
  - RX = PA10（本集未用）
- **USB 转 TTL 模块**：模块 **RX 接 PA9**、GND 与板子**共地**（TX/RX 要交叉）
- OLED: SCL = PB8、SDA = PB9（软件模拟 I2C，用于显示）

## 功能
- 串口参数 **9600 / 8 位数据 / 无校验 / 1 位停止位**（8N1）
- 提供 4 个发送函数：

| 函数 | 作用 |
|---|---|
| `Serial_SendByte(uint8_t Byte)` | 发送**一个字节**（最底层，其余函数都调它） |
| `Serial_SendArray(uint8_t *Array, uint32_t length)` | 发送**一个数组** |
| `Serial_SendString(char *String)` | 发送**一个字符串**（以 `\0` 结尾） |
| `Serial_SendNumber(uint32_t Number, uint8_t Length)` | 发送**一个数字**（按十进制逐位拆成字符） |

- 另提供 **`printf` 重定向**（自己实现 `fputc`）→ 可以直接 `printf("Num = %d\r\n", 666)`，串口助手显示 `Num = 666`
- `main.c` 当前实际调用：`printf("Num = %d\r\n", 666)`；之前的 `Serial_SendNumber(12345, 5)` 等测试调用保留为注释

## 学习点

### 1. USART 初始化是「四件事」，少一件都不工作
```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);   /* ① 开 USART1 时钟 */
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);    /* ① 开 GPIOA 时钟 */
GPIO_Init(...);                                          /* ② 配引脚 */
USART_Init(USART1, &USART_InitStructure);                /* ③ 配参数 */
USART_Cmd(USART1, ENABLE);                               /* ④ 使能 USART ← 本工程原先漏了 */
```
- **USART1 挂在 APB2 上**（不是 APB1）—— 用 `RCC_APB2PeriphClockCmd`，这一点新手最容易搞错
- **`USART_Init()` 只写寄存器、不"开机"**：它配的是 `BRR`（波特率）、`CR1`（字长/校验/模式）、`CR2`（停止位）、`CR3`（流控），**从不把 `CR1` 的 `UE`（USART Enable，bit13）置 1**
- **`USART_Cmd()` 才是把 `CR1 |= UE` 的那一步**。复位后 `UE = 0`，所以只调 `USART_Init` 等于"配置好了但没开机" —— **串口一个字节都发不出去**（TX 脚无任何波形）

### 2. 轮询发送：写数据 + 等标志
```c
USART_SendData(USART1, Byte);
while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
```
- `USART_SendData` 把字节写进**发送数据寄存器 `DR`**
- **`TXE`（Transmit Data Register Empty）**= "数据寄存器空了，可以放下一个"
- 先写再等：等到了就说明上一个字节已经被搬进移位寄存器、可以从容发下一个
- 这是**轮询（阻塞）**方式 —— 简单、不占中断，但发长数据会把 CPU 占住。以后会学中断/DMA 方式

### 3. 数字 → 字符：按位除权取余
```c
Number / Serial_Pow(10, Length - 1 - i) % 10 + '0'
```
- 从**最高位**开始拆：先用除法把高位**移下来**，再用 `% 10` 留下**那一位**，最后 `+ '0'` 变成 ASCII
- 例：`12345 / 10³ = 12`，`12 % 10 = 2` → 第 4 位是 `'2'`
- `Serial_Pow(10, 3) = 1000` 就是"权值"。**指数必须随循环变量 `i` 一起变**
- `+ '0'` 是 ASCII 码偏移：数字 `0` 的 ASCII 是 `0x30`，所以数值 `n` 对应字符 `'0' + n`

### 4. 为什么 TX 引脚要配成 `GPIO_Mode_AF_PP`（复用推挽）
- **正文的 GPIO 是 CPU 说了算**（读写 `ODR`）→ `Out_PP`
- **复用功能引脚是外设说了算**（这里是 USART 自己控制电平）→ `AF_PP`
- 配成 `Out_PP` 的话 USART 就管不到这个脚，发不出数据

### 5. 8N1 与 9600 的含义
- `9600` = 每秒 9600 个 bit，约 **1 bit ≈ 104 μs**
- `8` = 数据位 8 位（`USART_WordLength_8b`）
- `N` = 无校验（`USART_Parity_No`）
- `1` = 1 位停止位（`USART_StopBits_1`）
- 加上起始位，实际每传 1 字节要 10 bit → **约 960 字节/秒**
- **串口助手必须设成一样的参数**，否则全是乱码

### 6. `printf` 重定向：实现 `fputc`，并且**必须勾选 Use MicroLIB**

**原理**：`printf` 最后总要落到"把某一个字节输出出去"这一步。ARM Compiler 5 的 C 库在这一步调用 **`fputc`** —— 所以**只要自己实现一个 `fputc`，`printf` 的输出就全走你的函数**了。

```c
/* Serial.c */
#include <stdio.h>

int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);      /* 复用已有的单字节发送 */
    return ch;                /* 返回写出的字符（不是 EOF） */
}
```
```c
/* main.c */
printf("Num = %d\r\n", 666);
```

**验证过的调用链**（从 `Listings/prjoct.map` 里读出来的）：
```
main.o(i.main) → printf1.o(__2printf) → printfb.o(_printf_core) → serial.o(i.fputc) → Serial_SendByte
../clib/microlib/...        ← MicroLIB 确实在生效（map 里出现 39 处）
```

- ⚠️ **前提：`Options for Target → Target → Use MicroLIB` 必须勾选。**
  不勾选的话，用的是标准 C 库 —— 它的 `printf` 会牵扯到 **semihosting（半主机）**：库函数通过 `BKPT` 指令向调试器索要 I/O。**没接调试器时，程序会卡在里面出不来**（看起来就是"printf 不输出而且死机"）
- ⚠️ **MicroLIB 的 `printf` 不支持 `%f`（浮点）**。`%d / %u / %x / %c / %s` 都支持；要用浮点打印得另做处理
- ⚠️ **换行要发 `\r\n` 而不是只发 `\n`**：串口助手（以及很多终端）只认 `\r` 才是"回到行首"，只给 `\n` 会一直往右跑
- 💡 `Serial.h` 里也写了 `#include <stdio.h>` —— 这样**别的 .c 文件包含 Serial.h 后就能直接用 `printf`**（`FILE` 类型需要它）

## 调试记录（本工程上传前修改了 3 处）

### 修 1：`Serial_SendNumber` 的位数算错 —— 输出 `"22222"` 而不是 `"12345"`

**原代码**（`Serial.c`）：
```c
Serial_SendByte(Number / Serial_Pow(10, Length -1 -1) %10 + '0');
                                          /* 指数里应该是 i，写成了常量 1 */
```
**指数 `Length-1-1` 与循环变量 `i` 无关** → 每一位都算同一个数。江科大原版是 `Serial_Pow(10, Length - 1 - i)`。

PC 仿真实测（`main.c` 调用的 `Serial_SendNumber(12345, 5)`）：
```
指数恒为 5-1-1 = 3 → 每次都算 12345/1000%10 = 2
原版输出 : "22222"
修正输出 : "12345"
```

**⚠️ 它还藏着第二个更狠的后果**：`Serial_Pow(uint32_t x, uint32_t y)` 的 `y` 是**无符号**，指数为负会翻成天文数字：

| `Length` | `Length-1-1`（有符号） | 传进 `uint32_t` 后 |
|---|---|---|
| **1** | **-1** | **4294967295** ← `while(y--)` 循环 43 亿次 |

也就是说：**`Serial_SendNumber(任意数, 1)` 会让程序僵住** —— 在 72MHz 的 M3 上按每次迭代 5~7 个周期估算，**约 5~7 分钟**，看起来就像死机。

修正后 PC 仿真 5 个用例全通过：`(12345,5)→"12345"`、`(6789,4)→"6789"`、`(5,1)→"5"`（不再卡住）、`(0,3)→"000"`、`(123,3)→"123"`。

### 修 2：`Serial_Init()` 漏了 `USART_Cmd(USART1, ENABLE)`

原代码在 `USART_Init(USART1, &USART_InitStructure);` 之后就结束了。查本仓库库源码 `lib/stm32f10x_usart.c` 核实：

| 函数 | 写了哪些寄存器 | 是否使能 USART |
|---|---|---|
| `USART_Init` | `BRR`（波特率）、`CR1`（字长/校验/模式）、`CR2`（停止位）、`CR3`（流控） | ❌ **不置 `CR1` 的 `UE` 位** |
| `USART_Cmd` | `USARTx->CR1 \|= USART_CR1_UE_Set` | ✅ **这才是使能** |

`USART_Init` 用的 `CR1_CLEAR_Mask` 只是**保留** `UE` 位（不清零），但从不置 1；复位值为 0 → **`UE` 全程为 0**。结果：**串口助手什么都不显示**。

### 修 3：文件编码从 GBK 转成 UTF-8

上传前 `User/main.c` 和 `Hardwere/Serial.c` 是 **GBK（ISO-8859）编码**（在 Keil 里编辑保存时按中文 ANSI 存的），而本仓库其余 17 个工程都是 **UTF-8**。

**不转的话**：GitHub 按 UTF-8 解码 GBK 字节 → **整个文件的中文注释在网页上全是乱码**。

已用 `iconv -f GBK -t UTF-8` 转码，**纯内容零改动**（与原件逐行对比，除了上述两处修改外完全一致），行尾保持 CRLF 不变。

> 💡 建议把 Keil 的编辑器编码也设成 UTF-8（`Edit → Configuration → Editor → Encoding`），以后就不会再存出 GBK 文件。

## 已知边界（诚实标注）

- **`Serial_SendArray(uint8_t *Array, uint32_t length)` 的参数类型不匹配**：`length` 是 `uint32_t`，但循环变量 `i` 声明为 `uint16_t` → 若 `length > 65535`，`i` 永远到不了上界，会**死循环**。本工程只发短数组，不影响功能
- **`Serial_Pow()` 是内部辅助函数，却在 `Serial.h` 里导出**：它只为 `Serial_SendNumber` 服务，本可以写成 `static`（或干脆不放进头文件）。
  另外它用 `uint32_t y` 当"次数"参形，**传负数会翻成 43 亿**（见调试记录修 1）—— 这是这类"无符号当次数"写法的通用陷阱
- **`Serial.h` 的保护宏是 `__SERIAL__H`**：双下划线开头属 C 标准的保留标识符；本仓库其他文件用 `__XXX_H` 是全仓库通病
- **`main.c` 里前面的测试调用都被注释掉了**：`Serial_SendByte(0x41)`、`Serial_SendArray`、`Serial_SendString`、`Serial_SendNumber(12345, 5)` 都留着注释，当前只跑 `printf("Num = %d\r\n", 666)`。这是逐步测试的痕迹，不是死代码
- **`main.c` 里 `MyArray` 那两行注释已不能直接取消注释**：原本是 `// uint8_t MyArray[] = {0x41,0x42,0x43,0x44};`，2026-10-14 编辑时 `uint8_t` 被删掉了，现在是 `// MyArray[] = {...};` —— 残缺的注释代码，无编译影响
- **`KEY.c/h`、`LED.c/h` 未被任何文件引用**，未纳入仓库（只传工程实际用到的文件）
- **`EventRecorderStub.scvd` / `DebugConfig/` / `Objects/` / `Listings/`** 是 Keil 生成的调试与编译产物，未纳入

## 文件
- `main.c` — 主程序（调用 `printf("Num = %d\r\n", 666)`）
- **`Serial.c / Serial.h`** — ★ 本工程核心：串口初始化 + 4 个发送函数 + **`fputc` 重定向**
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
