#ifndef __PWM__H
#define __PWM__H

void PWM_Init(void);
void PWM_SetCompare1(uint16_t Compare);
void PWM_SetPrescaler(uint16_t Prescaler); //通过改变PSC,改变频率

#endif
