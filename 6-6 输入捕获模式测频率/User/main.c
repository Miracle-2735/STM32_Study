#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "IC.h"

int main(void)
{
	OLED_Init();
	PWM_Init();     // TIM2_CH1（PA0）发 PWM，当被测信号源
	IC_Init();      // TIM3_CH1（PA6）捕获，测频率
	
	OLED_ShowString(1, 1, "Freq:00000Hz");
	
	// 自测回环：PA0 用杜邦线接到 PA6
	PWM_SetPrescaler(720 - 1);       // 72M / 720 / 100 = 1kHz
	PWM_Setcompare1(50);             // 50 / 100 = 50%
	
	while(1)
	{
		OLED_ShowNum(1, 6, IC_GetFreq(), 5);    // 5 位，最多显示 99999Hz
	}
}
