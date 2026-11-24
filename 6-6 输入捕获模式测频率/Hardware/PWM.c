#include "stm32f10x.h"                  // Device header

/*
 * 本工程的 PWM 不是用来驱动东西的，而是当“被测信号源”：TIM2_CH1 从 PA0 发方波，
 * 用杜邦线接回 PA6 给 TIM3 的输入捕获去测，所以频率、占空比要能随时改。
 */
void PWM_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
//	GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	// 上面三行是备选方案：把 TIM2_CH1 重映射到 PA15。PA15 默认是 JTAG 的 JTDI 脚，
	// 必须同时关掉 JTAG 才能当普通 IO 用，麻烦且会失去 JTAG 调试，所以最终仍用默认的 PA0，这三行注释掉。
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;     // PWM 走片上定时器通道，必须配复用推挽，不能配普通推挽
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;      //GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	TIM_InternalClockConfig(TIM2);
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;       //ARR：周期 = 100 个计数，占空比分辨率 1%
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;     //PSC：72MHz / 720 = 100kHz 计数，配合 ARR 得 1kHz
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;      // PWM1：CNT < CCR 时输出有效电平（高）
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
	// 第三个参数必须是 Immediate（立即重载）：默认的 Update 要等下一次更新事件才装载新 PSC，
	// 改完频率会有一小段时间还发旧频率，OLED 上的读数也跟着慢半拍。
	TIM_PrescalerConfig(TIM2, Prescaler, TIM_PSCReloadMode_Immediate);
}