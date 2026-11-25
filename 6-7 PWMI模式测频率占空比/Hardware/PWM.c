#include "stm32f10x.h"                  // Device header

/*
 * 本工程的 PWM 当“被测信号源”用：TIM2_CH1 从 PA0 发方波，杜邦线接回 PA6 给 TIM3 的 PWMI 去测。
 * 与 6-6 的 PWM.c 完全相同，只是 main 里把频率改成 100Hz 方便观察占空比。
 */
void PWM_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
//	GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	// 备选方案：CH1 重映射到 PA15（要关 JTAG 才能用），本工程仍走默认的 PA0，故注释掉
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;     // 复用推挽，PWM 必须走片上定时器通道
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;      //GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	TIM_InternalClockConfig(TIM2);
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;       //ARR：100 个计数一个周期，占空比分辨率 1%
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;     //PSC：默认 720 得 1kHz，main 里再改成 7200 得 100Hz
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;    //CCR
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	
	TIM_Cmd(TIM2, ENABLE);
}

void PWM_Setcompare1(uint16_t Compare)
{
	TIM_SetCompare1(TIM2, Compare);
}

/* 运行时改预分频，即改输出频率：Freq = 72MHz / (PSC + 1) / (ARR + 1) */
void PWM_SetPrescaler(uint16_t Prescaler)
{
	// 用 Immediate 立即重载，改完频率当场生效；否则要等下一次更新事件才装载，读数会慢半拍
	TIM_PrescalerConfig(TIM2, Prescaler, TIM_PSCReloadMode_Immediate);
}
