#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "IC.h"

int main(void)
{
	OLED_Init();
	PWM_Init();     // TIM2_CH1（PA0）发 PWM，当被测信号源
	IC_Init();      // TIM3_CH1/CH2（PA6）PWMI 模式，测频率和占空比
	
	OLED_ShowString(1, 1, "Freq:00000Hz");
	OLED_ShowString(2, 1, "Duty:00%");
	
	// 自测回环：PA0 用杜邦线接到 PA6
	PWM_SetPrescaler(7200 - 1);      // 72M / 7200 / 100 = 100Hz
	PWM_Setcompare1(80);             // 80 / 100 = 80%（抓 20% 对照时改成 20）
	
	while(1)
	{
		OLED_ShowNum(1, 6, IC_GetFreq(), 5);    // 5 位频率
		OLED_ShowNum(2, 6, IC_GetDuty(), 2);    // 2 位占空比
	}
}
