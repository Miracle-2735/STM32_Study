# 5-1 对射式红外传感器计数

## 功能
对射式红外传感器被遮挡一次，计数值加 1，OLED 实时显示累计次数。

PB14 接 EXTI 外部中断、**只下降沿触发**；中断里只置一个标志位，计数和防连发延时全部放到主循环里做。

## 接线
- 传感器 DO（OUT）→ PB14
- VCC → 3.3V
- GND → GND
- OLED：SCL → PB8，SDA → PB9（显示用）

## 用到的代码
- `System/Delay.c/.h`：毫秒延时，现在**只在主循环里**用于防连发，不再进中断
- `Hardware/CountSensor.c/.h`
  - 开 GPIOB、AFIO 时钟，PB14 配上拉输入 IPU
  - `GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14)` 把 PB14 映射到 EXTI Line14
  - EXTI 配**只下降沿**触发（`EXTI_Trigger_Falling`），中断模式
  - NVIC 通道 `EXTI15_10_IRQn`（PB10~PB15 共用这一个中断号），抢占 1、子 1
  - `EXTI15_10_IRQHandler`：确认 `EXTI_Line14` 置位后，读一次 PB14 电平，为低才把 `Flag_Count` 置 1，然后清中断标志——**不做延时、不做计数**
  - 对外接口：`CountSensor_Init` / `CountSensor_Get`（读累计值）/ `CountSensor_Add`（加 1）/ `CountSensor_GetFlag` / `CountSensor_ClearFlag`
- `Hardware/OLED.c/.h`：显示计数
- `User/main.c`：`if (CountSensor_GetFlag() == 1) { Add(); ClearFlag(); Delay_ms(200); }`，然后刷新 `OLED_ShowNum`

## 11-22 这次改了什么（重点）

第一版（跟视频直接抄的那版）是这样：

```c
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line14) == SET)
    {
        Delay_ms(200);          // 消抖
        CountSensor_Count ++;   // 直接在这里计数
        Delay_ms(200);          // 防连发
        EXTI_ClearITPendingBit(EXTI_Line14);
    }
}
```

两个毛病：

1. **中断里塞了 400ms 延时**。中断服务函数应该尽量短。占着中断 400ms 期间 CPU 什么别的都干不了，OLED 不刷新，所有同优先级和低优先级的其他中断全被压着排队。
2. **双边沿触发会把一次遮挡计成两次**。遮挡时 DO 由高变低（下降沿，进中断），移开时由低变高（上升沿，又进一次中断）。只要两次之间超过了那 400ms 延时，一次遮挡就加了 2。

改完之后的职责分工：

| 谁 | 干什么 | 耗时 |
|---|---|---|
| 中断 `EXTI15_10_IRQHandler` | 只判断"有下降沿 + 引脚确实是低电平"，置 `Flag_Count = 1` | 几百纳秒 |
| 主循环 `while(1)` | 看到标志 → 加 1 → 清标志 → 延时 200ms 防连发 → 刷新 OLED | 不占用中断 |

一句话：**中断只负责"记下发生了"，主循环负责"处理这件事"**。这是江科大 PPT 里反复强调的中断使用原则，这次是自己动手踩过之后才真正理解的。

另外 `CountSensor.c` 里原来 `#include "Delay.h"` 也一起去掉了——中断里不再延时，这个模块就不该依赖 Delay 了。`main.c` 自己要用延时，所以 `Delay.h` 留在 `main.c` 里。

## 遇到的问题

- **EXTI 必须开 AFIO 时钟**：`RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE)`，否则引脚到 EXTI 线的映射不生效，中断根本进不来。
- PB14 属于 Line14，对应的中断服务函数名是 `EXTI15_10_IRQHandler`（10~15 共用一个），不是单独的 EXTI14 中断函数；进了这个函数还要用 `EXTI_GetITStatus(EXTI_Line14)` 确认是哪根线触发的。
- 中断里那句 `GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0`，能挡掉"沿已经触发、但等中断跑起来时引脚已经弹回高电平"的极短毛刺，但它**不是严格意义的消抖**，别指望它彻底解决机械/光学抖动。
- 若计数一直为 0：先查传感器 DO 有没有接到 PB14、模块供电和指示灯是否正常，再查 AFIO 时钟、EXTI 线映射、NVIC 是否使能。

## 待改进（发现了但还没改）

`ClearFlag()` 现在放在 `Delay_ms(200)` **之前**，那个 200ms 死区其实没真正生效：

> 遮挡 → 主循环加 1、清标志 → 进入 200ms 延时。延时期间如果又抖出一个下降沿，中断会把 `Flag_Count` 重新置 1 → 延时一结束，主循环立刻又加 1。

也就是说 200ms 内的抖动仍会被算成第二次遮挡。把清标志挪到延时**之后**，死区才成立：

```c
if (CountSensor_GetFlag() == 1)
{
    CountSensor_Add();
    Delay_ms(200);              // 先让它抖完
    CountSensor_ClearFlag();    // 再把这 200ms 内所有新置的标志一起清掉
}
```
