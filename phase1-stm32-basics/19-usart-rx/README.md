# 9-2 串口发送+接收

> 江科大 STM32 入门教程 · 第 9 章 USART 串口通信 · 第 2 集（发送 + **接收**）
> 在 9-1（`18-usart-send`）的基础上补上**接收**：PA10 输入 + RXNE 中断 + 标志位取数据。
> 本工程同时保留了 `printf` 重定向（`fputc`）。

## 硬件
- MCU: STM32F103C8T6
- **串口**: USART1
  - **TX = PA9**（`GPIO_Mode_AF_PP`，复用推挽输出 50MHz）
  - **RX = PA10**（`GPIO_Mode_IPU`，**上拉输入** —— 见学习点 1）
- **USB 转 TTL 模块**：接线**必须交叉**
  - 模块 **RX → PA9**（板子发、模块收）
  - 模块 **TX → PA10**（模块发、板子收）
  - 模块 **GND ↔ 板子 GND**（共地）
- OLED: SCL = PB8、SDA = PB9（软件模拟 I2C）

## 功能
- 串口参数 **9600 / 8 位数据 / 无校验 / 1 位停止位**（8N1）
- **发送**：4 个函数（`Serial_SendByte` / `SendArray` / `SendString` / `SendNumber`）+ **`printf` 重定向**
- **接收**：**RXNE 中断接收**，收到字节后置标志位；主循环查标志位取数据
  - `Serial_RxData` — 收到的字节
  - `Serial_Rxflag` — "有新数据"标志
  - `Serial_GetRxFlag()` — 查标志（**读到后自动清零**）
  - `Serial_GetRxData()` — 取数据
- `main.c`：OLED 第 1 行显示标签 `RxData:`，第 8 列起用**两位十六进制**显示收到的字节（如 `41`）

## 学习点

### 1. 接收要三件事，少一件都收不到
```c
/* ① 引脚：RX 配成"上拉输入" */
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;      /* RX 脚是输入，不是复用输出！ */
GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
GPIO_Init(GPIOA, &GPIO_InitStructure);

/* ② 模式：必须把 Rx 一起 OR 进去 */
USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;   /* ← 关键 */

/* ③ 使能 */
USART_Cmd(USART1, ENABLE);
```
- **`USART_Mode` 的位值**（库头文件）：`USART_Mode_Rx = 0x0004`（`CR1` 的 **RE** 位）、`USART_Mode_Tx = 0x0008`（`CR1` 的 **TE** 位）
- `USART_Init()` 内部是 `tmpreg |= ... | USART_Mode` —— **只有 `USART_Mode` 里带的位才会被置 1**
- 所以**只写 `USART_Mode_Tx` → `RE = 0` → 接收器被禁用 → `RXNE` 永远不会置位 → 一个字节也收不到**（本工程原先就是这个 bug，见调试记录修 1）
- `USART_Mode_Tx | USART_Mode_Rx = 0x000C`，`TE` 和 `RE` 都置 1，也通过库的 `IS_USART_MODE` 断言
- **RX 脚要配成 `GPIO_Mode_IPU`（上拉输入）**：串口空闲时线路是高电平，上拉能保证没接模块时引脚不会乱飘、不会误触发接收

### 2. 接收方式：**查询**和**中断**，只能选一个

| | 查询法 | 中断法（本工程用） |
|---|---|---|
| 怎么写 | `if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) { data = USART_ReceiveData(USART1); }` | 开 `USART_ITConfig(USART1, USART_IT_RXNE, ENABLE)` + NVIC，在 `USART1_IRQHandler` 里读 |
| 特点 | 简单、占 CPU（一直循环等），主循环里做别的事就不灵敏 | 不占 CPU、响应快，但要在中断里快速读完 |

**⚠️ 两者不能同时用！** 如果开了 RXNE 中断又在主循环里查 `RXNE` 标志：
- 字节到达 → `RXNE` 置位 → **中断几个周期内就冲进去** `USART_ReceiveData()`（**读 `DR` 就会清掉 `RXNE`**）
- 主循环轮到检查时 `RXNE` 已经被清了 → **主循环几乎永远看不到 `SET`**，那段查询代码等于白写

所以本工程的做法是：**中断里只做"收字节 + 置标志"，主循环查标志处理**：
```c
/* Serial.c —— 中断服务函数 */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        Serial_RxData = USART_ReceiveData(USART1);   /* 读 DR：取走数据并清 RXNE */
        Serial_Rxflag = 1;                           /* 通知主循环 */
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
```
```c
/* main.c —— 主循环 */
while (1)
{
    if (Serial_GetRxFlag() == 1)
    {
        RxData = Serial_GetRxData();
        OLED_ShowHexNum(1, 8, RxData, 2);
    }
}
```
```c
/* Serial.c —— 标志位"读后清零"，保证每个字节只被处理一次 */
uint8_t Serial_GetRxFlag(void)
{
    if (Serial_Rxflag == 1) { Serial_Rxflag = 0; return 1; }
    return 0;
}
```
- **为什么要 `Serial_GetRxFlag()` 这种"读后清零"**：它是把"中断收数"和"主循环处理"解耦的最小机制 —— 中断只负责"登记一件事发生了"，主循环按自己的节奏去取，**取过就销账，不会重复处理**
- **注意它只是一个标志位，不是队列**（见「已知边界」）

### 3. `USART_GetFlagStatus` 与 `USART_GetITStatus` 的区别
- `USART_GetFlagStatus(USART1, USART_FLAG_RXNE)` — 读的是**状态寄存器 `SR` 的标志位**，是硬件状态
- `USART_GetITStatus(USART1, USART_IT_RXNE)` — 读的是**"该事件是否产生了中断"**，它同时看 `SR` 的标志位**和 `CR1` 里对应的中断使能位**
- **在中断服务函数里应该用 `GetITStatus`**：只有"标志位 + 中断使能"都满足才进来，语义正确；`GetFlagStatus` 只反映硬件状态，可能被别的事件带进来
- 本工程两处用法都对：**主循环里查标志位用的是 `Serial_GetRxFlag()`（自定义标志），中断里用的是 `GetITStatus`**

### 4. `RXNE` 是靠"读 `DR`"清除的
- `USART_ReceiveData(USART1)` 做的是**读 `DR` 寄存器** → 硬件自动把 `RXNE` 清 0
- 之后再 `USART_ClearITPendingBit(USART1, USART_IT_RXNE)` 是**双保险**（读 `DR` 已清标志，多清一次无害）
- 记住这条：**`RXNE` 不像别的标志那样要"写 1 清 0"，读一次数据就没了**

### 5. NVIC 优先级分组
```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);      /* 2 位抢占 + 2 位响应 */
NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;    /* 抢占优先级 */
NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           /* 响应优先级 */
```
- **`NVIC_PriorityGroupConfig` 是全局设置**（改的是内核 `SCB->AIRCR` 的 `PRIGROUP` 字段），**整个工程只应该设一次**
- 本工程把它放在 `Serial_Init()` 里（教程写法）。**以后用多个外设中断时，建议挪到 `main()` 开头统一设一次** —— 否则每个驱动初始化都重设一遍分组，后面初始化的会悄悄改掉前面中断的优先级语义（见「已知边界」）
- `NVIC_PriorityGroup_2` = 4 个优先级里，高 2 位给抢占、低 2 位给响应 → 抢占比响应更"硬"，抢占高的能打断抢占低的

### 6. 接线为什么必须交叉
- 串口的 TX 和 RX 都是**单向**的：TX 是输出、RX 是输入
- 所以 **A 的 TX 必须接 B 的 RX**，"TX 接 TX"两边都是输出 → 顶牛，谁也收不到
- 模块和板子**必须共地**，否则没有共同的 0V 参考，电平判断全是错的

## 调试记录（本工程上传前修改了 3 处）

### 修 1：`USART_Mode` 漏了 `USART_Mode_Rx` —— 一个字节都收不到

原代码：`USART_InitStructure.USART_Mode = USART_Mode_Tx;`

库头文件 `lib/stm32f10x_usart.h`：
```c
#define USART_Mode_Rx   ((uint16_t)0x0004)    /* CR1 的 RE 位 */
#define USART_Mode_Tx   ((uint16_t)0x0008)    /* CR1 的 TE 位 */
#define IS_USART_MODE(MODE) ((((MODE) & (uint16_t)0xFFF3) == 0x00) && ((MODE) != (uint16_t)0x00))
```
库源码 `lib/stm32f10x_usart.c` 的 `USART_Init()`：
```c
tmpreg |= (uint32_t)USART_InitStruct->USART_WordLength | USART_InitStruct->USART_Parity |
          USART_InitStruct->USART_Mode;      /* 只有 USART_Mode 带的位才置 1 */
```

→ 只给 `USART_Mode_Tx` ⇒ **`RE`（接收使能）位保持 0 ⇒ 接收器被禁用 ⇒ `RXNE` 永不置位 ⇒ 收不到任何数据**。
PA10 输入配了、RXNE 中断开了、NVIC 也配了，**但 USART 本身还是"只能发不能收"**。

✅ 已改为 `USART_Mode_Tx | USART_Mode_Rx`（= `0x000C`，`TE` 与 `RE` 都置 1，且通过 `IS_USART_MODE` 断言校验）。

> 这跟 9-1 漏 `USART_Cmd` 是**同一类错误**：外设"配置好了"和"真正使能了"是两件事。

### 修 2：主循环用查询法，与已开启的 RXNE 中断**互相打架**

原 `main.c`：
```c
if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)   /* ← 几乎永远不成立 */
{
    RxData = USART_ReceiveData(USART1);
    OLED_ShowNum(1, 1, RxData, 1);
}
```
但 `Serial_Init()` 里已经开了 `USART_ITConfig(USART1, USART_IT_RXNE, ENABLE)` + NVIC。
中断会在 `RXNE` 置位后几个周期内先跑，`USART_ReceiveData()` 读走 `DR` 并清掉 `RXNE` → **主循环看不到标志，OLED 不刷新**。
而且 `Serial.c` 里其实**已经写好了**整套中断接收（`Serial_Rxflag` / `Serial_GetRxFlag` / `Serial_GetRxData` / `USART1_IRQHandler`）—— 只是 `main.c` 没用上。

✅ 已改为查自定义标志位：
```c
if (Serial_GetRxFlag() == 1)
{
    RxData = Serial_GetRxData();
    ...
}
```

### 修 3：`OLED_ShowNum(1, 1, RxData, 1)` 只显示**最后一位**

`OLED.c` 的实现：
```c
OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
```
`Length = 1` → 只取个位。所以收到 `'A'`（`0x41` = **65**）会显示 `5`；收到换行 `\n`（**10**）会显示 `0`。
`UTF-8` 字符更是完全看不出收了什么。

✅ 已改为 **两位十六进制**，并加标签：
```c
OLED_ShowString(1, 1, "RxData:");        /* 初始化时显示一次 */
...
OLED_ShowHexNum(1, 8, RxData, 2);        /* 收到 'A' 显示 41 */
```

## 已知边界（诚实标注）

- **`Serial_Rxflag` 只是一个"标志位"，不是队列**：如果主循环还没取走上一个字节，新字节就到达，`Serial_RxData` 会被**直接覆盖**（只保留最后一个）。也就是说**连续快速发送的字节会丢**。现在的 `while(1)` 循环处理很快，手动在串口助手里逐字节发没问题；要做到"一个不丢"需要环形缓冲区（后续章节 / 进阶内容）
- **`NVIC_PriorityGroupConfig()` 被放在 `Serial_Init()` 里**（教程写法）：它是**全局**设置，多个外设驱动各自调用时会互相覆盖。建议以后挪到 `main()` 开头只设一次
- **`Serial.h` 里导出了 `USART1_IRQHandler(void)` 的原型**：中断服务函数由**启动文件的向量表**调用，不需要（也不应该）在头文件里声明。留着不影响编译，是多余的
- **继承了 9-1 的两处已知问题**：
  - `Serial_SendArray(uint8_t *Array, uint32_t length)` 的 `length` 是 `uint32_t`，循环变量 `i` 却是 `uint16_t` → `length > 65535` 会**死循环**
  - `Serial_Pow()` 是内部辅助函数却在头文件导出；它用 `uint32_t` 当"次数"形参，**传负数会翻成 43 亿**（详见 `18-usart-send` 的调试记录）
- **`Serial.h` 的保护宏是 `__SERIAL__H`**：双下划线开头属 C 标准保留标识符（全仓库通病）
- **`printf` 依赖「Use MicroLIB」勾选**：本工程编译产物 `.map` 里出现 `../clib/microlib/...`，说明 MicroLIB 已生效（详见 `18-usart-send` 学习点 6）。MicroLIB 的 `printf` **不支持 `%f`**
- **`KEY.c/h`、`LED.c/h` 未被任何文件引用**，未纳入仓库
- **`EventRecorderStub.scvd` / `DebugConfig/` / `Objects/` / `Listings/`** 是 Keil 生成的调试与编译产物，未纳入

## 文件
- `main.c` — 主程序（OLED 显示标签 + 循环查标志位取接收数据）
- **`Serial.c / Serial.h`** — ★ 本工程核心：串口初始化（**含接收配置**）+ 4 个发送函数 + `fputc` + **中断接收 + 标志位接口**
- `OLED.c / OLED.h / OLED_Font.h` — OLED 驱动（教程提供，软件 I2C 位时序 + 字库）
- `Delay.c / Delay.h` — 软件延时（复用 3-x）
- `stm32f10x_it.c / stm32f10x_conf.h` — 工程模板（中断向量 + 库配置）
  - `stm32f10x_it.c` 里的 `USART1_IRQHandler` 是**注释状态**，真正的 ISR 在 `Serial.c` 里 —— **不会重复定义**
