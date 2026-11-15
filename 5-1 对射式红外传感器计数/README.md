# 5-1 对射式红外传感器计数

## 功能
对射式红外传感器被遮挡一次，计数值加 1，OLED 实时显示累计次数。传感器输出接 EXTI 外部中断引脚，电平跳变时进中断计数。

## 接线
- 传感器 DO（OUT）→ PB14
- VCC → 3.3V
- GND → GND
- OLED：SCL → PB8，SDA → PB9（显示用）

## 用到的代码
- `System/Delay.c/.h`：中断里做消抖/防连发延时
- `Hardware/CountSensor.c/.h`：`CountSensor_Init`、`CountSensor_Get`
  - 开 GPIOB 和 AFIO 时钟，PB14 配上拉输入 IPU
  - `GPIO_EXTILineConfig` 把 PB14 映射到 EXTI Line14
  - EXTI 配双边沿触发（Rising_Falling），中断模式
  - NVIC 通道 `EXTI15_10_IRQn`（PB10~PB15 共用这一个中断号），抢占 1、子 1
  - 中断函数 `EXTI15_10_IRQHandler`：判断 `EXTI_Line14` 置位后，延时 200ms、`CountSensor_Count++`、再延时 200ms，最后 `EXTI_ClearITPendingBit` 清标志
- `Hardware/OLED.c/.h`：显示计数
- `User/main.c`：初始化 OLED 和传感器，while 循环 `OLED_ShowNum` 显示 `CountSensor_Get()`

## 遇到的问题
- **EXTI 必须开 AFIO 时钟**：`RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE)`，否则引脚到 EXTI 线的映射不生效，中断进不来。
- PB14 属于 Line14，对应的中断服务函数名是 `EXTI15_10_IRQHandler`（10~15 共用），不是单独的 EXTI14 中断函数；中断里还要再用 `EXTI_GetITStatus(EXTI_Line14)` 确认是哪根线触发的。
- 计数不准/一次遮挡跳多个数：对射红外和机械结构有抖动，加上遮挡手法快慢不一，属于正常现象；代码里用两段 `Delay_ms(200)` 在中断内做防连发，挡住期间不会反复加。
- 中断里不要写太长的逻辑，本工程靠短延时消抖；复杂消抖更推荐在主循环里做状态判断。
- 若计数一直为 0：先查传感器 DO 有没有接到 PB14、模块供电和指示 LED 是否正常，再查 EXTI 线映射和 NVIC 是否使能。
