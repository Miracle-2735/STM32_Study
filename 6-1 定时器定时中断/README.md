# 6-1 定时器定时中断

## 功能
TIM2 用**内部时钟**定时，每隔 1 秒进一次更新中断，中断里 `Num++`，OLED 显示累计秒数。

## 接线
- 本实验靠单片机内部时钟定时，不接外部外设
- OLED：SCL → PB8，SDA → PB9（显示用）

## 用到的代码
- `System/Timer.c/.h`：`Timer_Init`
  - 开 TIM2 时钟（TIM2 在 APB1 总线上），`TIM_InternalClockConfig(TIM2)` 用内部时钟
  - 时基配置：`TIM_Prescaler = 7200 - 1`、`TIM_Period = 10000 - 1`，向上计数
  - 时钟 72MHz，分频 7200 后计数频率 10kHz，数 10000 次正好 1 秒
  - `TIM_ClearFlag` 清初始更新标志，`TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE)` 开更新中断
  - NVIC 通道 `TIM2_IRQn`，抢占 2、子 1，`TIM_Cmd` 启动定时器
  - 文件末尾只留一行注释说明“中断函数在 main.c 里”，本文件不定义 `TIM2_IRQHandler`
- `User/main.c`：定义全局 `uint16_t Num`，初始化 OLED 和定时器，while 循环显示 Num；**中断服务函数 `TIM2_IRQHandler` 写在 main.c 里**，判断更新中断置位后 `Num++` 再清标志
- `Hardware/OLED.c/.h`：显示

## 遇到的问题
- **中断函数只能定义一处**：本工程真正的中断函数放在 `main.c`（和 `Num` 定义在同一个文件里），`Timer.c` 只留注释指引。早期 `Timer.c` 末尾曾有一段被 `/* */` 包住的占位 `TIM2_IRQHandler`，两处都放开会在链接时报中断函数重复定义；11-22 已把这段注释块整体删掉，避免以后误放开。
- 定时周期由 PSC（分频系数）和 ARR（自动重装载值）共同决定：改这两个数就能改定时时长，别只改其中一个。
- 初始化后先 `TIM_ClearFlag` 清一次更新挂起标志，否则开机可能立刻进一次中断。
- 更新中断标志必须在中断里 `TIM_ClearITPendingBit` 清掉，否则退出中断后会反复进中断。
- 这里的定时是定时器硬件自动累加计数，和 3-1 那种 `Delay_ms` 空转延时不同：定时期间 CPU 可以干别的事，到点才被中断打断。

## 更新记录（2026-11-22）

| 文件 | 改动 | 说明 |
|---|---|---|
| `System/Timer.c` | 删除文件末尾被 `/* */` 包住的占位 `TIM2_IRQHandler`；补全初始化各步注释和 1 秒定时推算 | 占位中断函数是隐患（误放开就重复定义），删除后中断只在本工程 main.c 一处 |
| `User/main.c` | 删除调试用的 `//OLED_ShowNum(2, 5, TIM_GetCounter(TIM2), 5)` 注释行；补充主循环与中断函数注释 | 逻辑未变，仍是 1 秒进一次中断、`Num++` |
| 本 README | 补充上述注释说明 | — |
