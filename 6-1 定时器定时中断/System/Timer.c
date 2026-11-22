#include "stm32f10x.h"                  // Device header

// 定时器初始化：配置 TIM2 产生 1 秒一次的更新中断
void Timer_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);   // 开启 TIM2 时钟（TIM2 挂在 APB1 总线上）

	TIM_InternalClockConfig(TIM2);    // 选择内部时钟作为定时器计数时钟（默认就是内部时钟，这里显式指定）

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 时钟分频：定时器输入采样时钟不分频
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数模式：从 0 累加，计到重装值后溢出归零
	TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;               // 自动重装载值 ARR：计数到 9999 溢出，一轮共 10000 个计数
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;             // 预分频器 PSC：72MHz / 7200 = 10kHz，计数脉冲 0.1ms 一次
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;             // 重复计数器：仅高级定时器使用，通用定时器填 0
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

	// 定时时间推算：计数频率 10kHz × 周期 10000 个计数 = 10000 / 10000 = 1s，即每 1 秒溢出一次
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);    // 先清一次更新标志，避免定时器刚开启时就误进中断
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); // 使能 TIM2 的更新中断

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // NVIC 优先级分组 2（抢占 4 级、子 4 级）

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;              // TIM2 中断通道
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 2; // 抢占优先级 2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           // 子优先级 1
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM2, ENABLE);   // 启动 TIM2，计数器开始累加
}

/*
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{

		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
*/
