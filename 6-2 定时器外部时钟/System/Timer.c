#include "stm32f10x.h"                  // Device header

// 定时器初始化：把 TIM2 配成“外部时钟模式 2”，用 PA0（TIM2_ETR）上的脉冲直接驱动硬件计数器
// 中断服务函数 TIM2_IRQHandler 写在 User/main.c 里，和 Num 的定义放一起，不需要 extern
void Timer_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    // 开启 TIM2 时钟（TIM2 挂在 APB1）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);   // 开启 GPIOA 时钟（PA0 挂 APB2）

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;     // 上拉输入：传感器没输出时引脚保持高电平，避免悬空乱计数
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;         // PA0 复用为 TIM2_ETR 外部计数输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// 外部时钟模式 2：ETR 引脚每来一个边沿，硬件 CNT 自动 +1，不占用中断、也不消耗 CPU
	// 第 4 个参数是 ETR 滤波器档位，取值 0x00 ~ 0x0F（函数内部会把它左移 8 位写进 SMCR 的 ETF 位）：
	//   0x00 = 不滤波，原始信号直接计数，有毛刺就乱跳（之前 CNT 在 5、7 附近晃就是这个原因）
	//   0x0F = 最高档：采样时钟 f_DTS/32（72MHz/32 ≈ 2.25MHz，约 0.44μs 采一次），
	//          要连续 8 次采样电平一致才认这个边沿 → 信号必须稳定约 3.5μs 才算有效，
	//          能把对射传感器的窄毛刺滤掉；代价是外部脉冲最快只能到约 140kHz，本实验远用不到这么快
	TIM_ETRClockMode2Config(TIM2, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0x0F);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 时钟分频不分频，f_DTS = 72MHz
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 10 - 1;                   // ARR = 9：CNT 数到 9 溢出回 0，每 10 个脉冲溢出一次
	TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;                 // PSC = 0：外部来的脉冲不再分频，来一个算一个
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;             // 重复计数器：通用定时器填 0
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);        // 先清一次更新标志，避免初始化瞬间误进一次中断
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);   // 使能更新中断（CNT 溢出时触发）

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);   // NVIC 分组 2（抢占 4 级、子 4 级）

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;              // TIM2 中断通道
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 2; // 抢占优先级 2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           // 子优先级 1
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM2, ENABLE);   // 启动 TIM2，开始等 ETR 脉冲
}

// 读硬件计数器 CNT 的当前值（0 ~ ARR），主循环用来显示“现在数到第几个脉冲”
uint16_t Timer_GetCounter(void)
{
	return TIM_GetCounter(TIM2);
}
