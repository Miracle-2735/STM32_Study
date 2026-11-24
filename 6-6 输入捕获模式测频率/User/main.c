#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "IC.h"

int main(void)
{
	OLED_Init();
	PWM_Init();     // TIM2_CH1（PA0）输出 PWM，充当“已知频率”的被测信号源
	IC_Init();      // TIM3_CH1（PA6）输入捕获，测这个信号的频率
	
	OLED_ShowString(1, 1, "Freq:00000Hz");   // 先把固定文字画出来，数字几位留给 ShowNum 反复刷新
	
	// 自测回环：PA0 用杜邦线短接到 PA6，自己发、自己测，不依赖信号发生器
	PWM_SetPrescaler(720 - 1);       // Freq = 72M / (PSC + 1) / (ARR + 1) = 72M / 720 / 100 = 1kHz
	PWM_Setcompare1(50);             // Duty = CCR / (ARR + 1) = 50 / 100 = 50%
	
	while(1)
	{
		// 第 1 行第 6 列起显示 5 位数字（不满 5 位前面补 0）；ShowNum 只刷新数字区，左边 “Freq:” 不会被擦掉
		// 注意 5 位最多显示 99999Hz，测到 10 万赫兹以上会显示不全，要测得更高就加大位数
		OLED_ShowNum(1, 6, IC_GetFreq(), 5);
	}
}