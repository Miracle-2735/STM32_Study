#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Timer.h"

uint16_t Num;   // 秒计数：在 TIM2 中断里自增，主循环刷新到 OLED 显示

int main(void)
{
	OLED_Init();
	Timer_Init();

	OLED_ShowString(1, 1, "Num:");

	while(1)
	{
		OLED_ShowNum(1, 5, Num, 5);   // 主循环只负责刷新显示，计数动作在定时器中断里完成
	}
}

// TIM2 更新中断服务函数：每 1 秒进入一次
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)  // 确认是更新中断触发
	{
		Num ++;                                        // 秒计数 +1
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);    // 清除中断挂起标志，否则退出后会反复进中断
	}
}
