#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Timer.h"

uint16_t Num;   // 溢出次数：CNT 每数满 10 个脉冲回 0 一次，中断里 Num +1，所以 Num ≈ 总脉冲数 / 10

int main(void)
{
	OLED_Init();
	Timer_Init();

	OLED_ShowString(1, 1, "Num:");
	OLED_ShowString(2, 1, "CNT:");

	while(1)
	{
		OLED_ShowNum(1, 5, Num, 5);                      // 第一行：溢出次数（软件统计）
		OLED_ShowNum(2, 5, Timer_GetCounter(), 5);       // 第二行：硬件计数器当前值（0~9 循环）
	}
}

// TIM2 更新中断服务函数：CNT 每溢出一次（数满 10 个脉冲）进来一次
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)   // 确认是更新中断
	{
		Num ++;                                       // 溢出次数 +1
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);   // 清标志，否则退出后反复进中断
	}
}
