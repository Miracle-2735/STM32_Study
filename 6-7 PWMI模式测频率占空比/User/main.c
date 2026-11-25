#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "IC.h"

int main(void)
{
	OLED_Init();
	PWM_Init();     // TIM2_CH1（PA0）发 PWM，当被测信号源
	IC_Init();      // TIM3_CH1/CH2（PA6）PWMI 模式，同时测频率和占空比
	
	OLED_ShowString(1, 1, "Freq:00000Hz");   // 第 1 行固定文字 + 5 位频率数字
	OLED_ShowString(2, 1, "Duty:00%");       // 第 2 行固定文字 + 2 位占空比数字
	
	// 自测回环：PA0 用杜邦线短接到 PA6
	PWM_SetPrescaler(7200 - 1);      // Freq = 72M / (PSC + 1) / (ARR + 1) = 72M / 7200 / 100 = 100Hz
	PWM_Setcompare1(80);             // Duty = CCR / (ARR + 1) = 80 / 100 = 80%
	                                 // 抓 20% 对照图时把这句改成 20，屏上 Duty 应随之变 20%，见 README 的"20% 对照"
	
	while(1)
	{
		OLED_ShowNum(1, 6, IC_GetFreq(), 5);    // 第 1 行第 6 列起 5 位：频率
		OLED_ShowNum(2, 6, IC_GetDuty(), 2);    // 第 2 行第 6 列起 2 位：占空比（%，两位数够用）
	}
}
