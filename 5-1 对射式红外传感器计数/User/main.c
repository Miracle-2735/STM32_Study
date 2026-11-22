#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "CountSensor.h"

int main(void)
{
    OLED_Init();
    CountSensor_Init();
    OLED_ShowString(1, 1, "Count:");

	while(1)
	{
		if(CountSensor_GetFlag() == 1)
		{
			CountSensor_Add();    // 替换原来的 CountSensor_Count++
			Delay_ms(200);       // 先让抖动/重复遮挡过去，进入死区
			CountSensor_ClearFlag(); // 死区结束再清标志，这 200ms 内新置的标志一并吞掉
		}
		OLED_ShowNum(1, 7, CountSensor_Get(), 5);
	}

}
