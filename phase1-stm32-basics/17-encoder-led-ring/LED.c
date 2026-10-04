#include "stm32f10x.h"                  // Device header
#include "LED.h"

/* ---- 硬件宏：引脚/端口只在这里改一次，别散落在函数里 ---- */
#define LED_PORT    GPIOA
#define LED_RCC     RCC_APB2Periph_GPIOA
#define LED_PINS    (GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5)

void LED_Init(void)
{
	/* APB2 总线上的外设，时钟不使能它就不工作 */
	RCC_APB2PeriphClockCmd(LED_RCC, ENABLE);

	GPIO_InitTypeDef s = {0};           /* 必须 = {0}，否则没赋的字段是栈上的随机值 */
	s.GPIO_Mode  = GPIO_Mode_Out_PP;    /* 推挽输出：能主动输出高和低 */
	s.GPIO_Pin   = LED_PINS;
	s.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_PORT, &s);

	LED_OffAll();                       /* 上电先全灭，避免灯乱亮 */
}

void LED_OffAll(void)
{
	/* ★1 灯是「低电平点亮」，那"全灭"要把 LED_PORT 的 LED_PINS 拉高还是拉低？
	      提示：标准库给的是 GPIO_SetBits / GPIO_ResetBits 这一对。
	      写完后问自己：如果用 GPIO_WriteBit(..., Bit_SET) 行不行？（README 里有答案） */

	/* >>> 在这里写一行 <<< */
	GPIO_SetBits(GPIOA, LED_PINS);
	
	
}

void LED_ShowOne(uint8_t pos)
{
	/* ★2 只让第 pos 颗亮，分两步：
	        第一步：全灭（直接调用上面的函数）
	        第二步：只把第 pos 位拉低。
	                "只有第 pos 位是 1、其余是 0" 的掩码怎么写？
	                提示：1 左移 pos 位 —— 1 << pos
	                然后：拉低用的是哪个库函数？它第二个参数要的是「哪几位」还是「第几位」？
	                （看 stm32f10x_gpio.h 里 GPIO_ResetBits 的第二个参数类型，是 uint16_t） */

	/* >>> 第一步 <<< */
	LED_OffAll();
	/* >>> 第二步 <<< */
	GPIO_ResetBits(GPIOA, 1 << pos);
	
	/* ★3（可选）如果 pos 传进来是 200 会怎样？1 << 200 是什么？
	      在下面写一句注释，说明你的做法：在 main 里保证，还是这里加防御判断。 */
}
