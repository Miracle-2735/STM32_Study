#include "stm32f10x.h"                  // Device header

/*
 * 输入捕获测频率（IC = Input Capture）
 * 思路：外部方波从 PA6（TIM3_CH1）进来，用“从模式复位”让每个上升沿把 CNT 硬件清零，
 *       于是 CH1 捕获到的 CCR1 就直接等于一个完整周期数了多少个计数，频率 = 计数时钟 / 周期。
 * 本工程计数时钟取 1MHz（PSC = 71），即 1 个计数 = 1μs，读出来天生就是微秒数。
 */
void IC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   // 上拉输入：捕获脚是输入，信号没接时也不会悬空乱跳
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;       // PA6 默认映射就是 TIM3_CH1，不需要开 AFIO 做重映射
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	TIM_InternalClockConfig(TIM3);                  // 计数器本身仍用内部 72MHz 时钟，被捕获的只是外部信号的电平跳变
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;    // ARR 取满：最多数 65536 个计数，低频信号也不至于中途溢出
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;    // PSC：72MHz / 72 = 1MHz，一个计数正好 1μs
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
	
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;              // 用 CH1
	TIM_ICInitStructure.TIM_ICFilter = 0xF;                       // 输入滤波最高档（f_DTS/32 采样、连续 8 次电平一致才认），抑制毛刺误触发
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;    // 上升沿捕获
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;         // 不分频：每个上升沿都要捕获，分频了测到的就不是一个周期
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;  // 直连：CH1 接 TI1（PA6），不走交叉通道
	TIM_ICInit(TIM3, &TIM_ICInitStructure);
	
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);         // 触发源选 TI1FP1（就是 CH1 那路滤波后的信号）
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);      // 从模式复位：来一个触发就把 CNT 清零，CCR1 因此直接等于一个完整周期
	
	TIM_Cmd(TIM3, ENABLE);                               // 启动 TIM3，开始捕获
}

/* 读频率：CCR1 是两次上升沿之间的计数值，计数时钟 1MHz，所以单元是 μs */
uint32_t IC_GetFreq(void)
{
	// +1 的来历：从模式复位让 CNT 在上升沿那一刻清零，捕获到的值比真实周期少 1 个计数，补上 1 才是完整周期。
	// 不加 1 的话，1kHz 这种整数频率会读成 1001Hz 一类偏大的值（约偏 0.1%）。
	// 注意这里的 1000000 是“1 秒有多少个计数”，跟着 PSC=71 走；改 PSC 必须同步改这个数。
	return 1000000 / (TIM_GetCapture1(TIM3) + 1);
}