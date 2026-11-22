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
			CountSensor_ClearFlag();
			Delay_ms(200);
		}
		OLED_ShowNum(1, 7, CountSensor_Get(), 5);
	}

}
