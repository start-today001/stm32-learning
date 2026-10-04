#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "Encoder.h"
#include "OLED.h"

int main(void)
{
	
	int16_t pos_raw = 0;        /* 编码器累计转过的计数，可以为负 */
	uint8_t pos;                /* 送进 LED 的位置，必须落在 0 ~ LED_NUM-1 */
	int16_t step1, step2;      /* 声明放这儿 */
	
	
	OLED_Init();
	LED_Init();
	Encoder_Init();
	/* 注意：第 1 步不要调用 Timer_Init()。
	   因为覆盖 main.c 后 TIM2 的中断服务函数没了，中断一开就跑到默认死循环。 */

	OLED_ShowString(1, 1, "Pos:");

	while (1)
	{
		pos_raw += Encoder_Get(); 
		/* ★4 增量累加。
		      Encoder_Get() 返回的是「自上次调用以来转过的计数」，读一次就清零
		      （它内部是 读 CNT → 把 CNT 写 0）。
		      所以位置要靠你自己累加出来。
		      正转为正、反转为负 —— 这一步不需要你做任何判断。 */
		step1 = pos_raw /4 % LED_NUM;	/* 赋值留在循环里 */
		step2 = step1 + LED_NUM;
		pos   = step2 % LED_NUM;
		

		/* ★5 环形回绕：把 pos_raw 映射到 0 ~ LED_NUM-1。
		      ⚠️⚠️ 这是本练习唯一的真坑：
		           C 语言里 (-1) % 6 的结果是 -1，不是 5。
		           pos 是 uint8_t，把一个 -1 塞进去会变成 255，
		           GPIO_ResetBits(LED_PORT, 1 << 255) 在 16 位参数上是未定义行为 ——
		           表现就是灯乱亮、或者转到负位置时灯全灭。
		      正确的做法是让余数永远为正：
		           先 % LED_NUM 得到 -5~5，再加一个 LED_NUM 变成正数，再 % 一次。
		      自己把这两步合起来写成一行。 */

		

		LED_ShowOne(pos);
		OLED_ShowSignedNum(1, 5, pos_raw, 5);   /* 显示原始累加值，方便你观察负数和回绕 */
	}
}
