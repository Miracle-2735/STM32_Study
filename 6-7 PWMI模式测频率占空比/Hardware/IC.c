#include "stm32f10x.h"                  // Device header

/*
 * PWMI 模式（PWM Input）：一个定时器、一根信号线，同时测出频率和占空比
 *   CH1：上升沿 + 直连 TI1  → 捕获整个周期      （CCR1）
 *   CH2：下降沿 + 间接映射 TI1 → 捕获高电平时间  （CCR2）
 * 两个通道吃的是同一路输入 PA6，所以外部只接一根线。
 * 与 6-6 的唯一区别：这里用 TIM_PWMIConfig 一次把 CH1、CH2 都配好，不用自己再配一遍 CH2。
 */
void IC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   // 上拉输入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;       // PA6 = TIM3_CH1（CH2 内部间接借用同一路 TI1，不需要第二根线）
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	TIM_InternalClockConfig(TIM3);                  // 计数器用内部 72MHz，被捕获的只是外部信号的跳变
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;    // ARR 取满：周期值再大也不容易溢出
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;    // PSC：72MHz / 72 = 1MHz，一个计数 1μs
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
	
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;              // 只写 CH1 一套参数，CH2 由库函数自动配成“相反的那一套”
	TIM_ICInitStructure.TIM_ICFilter = 0xF;                       // 输入滤波最高档，抑制毛刺
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;    // CH1 上升沿（周期起点）
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;         // 不分频，两个通道都一样
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;  // CH1 直连 TI1
	TIM_PWMIConfig(TIM3, &TIM_ICInitStructure);                   // 关键：它会把 CH2 自动配成 下降沿 + IndirectTI，两通道共用 TI1
	
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);         // 触发源 TI1FP1（CH1 那路）
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);      // 从模式复位：每次上升沿把 CNT 清零，CCR1/CCR2 都从同一个起点数起
	
	TIM_Cmd(TIM3, ENABLE);
}

/* 频率：CCR1 是一个完整周期的计数（1μs/计数） */
uint32_t IC_GetFreq(void)
{
	// +1 补偿复位与捕获同一时刻造成的 1 个计数偏差（同 6-6），否则整数频率会偏大 0.1% 左右
	return 1000000 / (TIM_GetCapture1(TIM3) + 1);
}

/* 占空比：CCR2 是上升沿到下降沿的计数（高电平时间），CCR1 是整个周期 */
uint32_t IC_GetDuty(void)
{
	// 先乘 100 再除，尽量保住精度，但整数除法仍会把小数截断：
	// 80% 的占空比可能显示 79 或 80，这是显示分辨率 1% 的正常现象，不是测量错误。
	// 想要 0.1% 分辨率就改成 (CCR2+1)*1000/(CCR1+1)，再自己插小数点。
	return (TIM_GetCapture2(TIM3) + 1) * 100 / (TIM_GetCapture1(TIM3) + 1);
}