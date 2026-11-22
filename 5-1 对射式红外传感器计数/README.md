# 5-1 对射式红外传感器计数

## 功能
对射式红外传感器被遮挡一次，计数值加 1，OLED 实时显示累计次数。

PB14 接 EXTI 外部中断、**只下降沿触发**；中断里只置一个标志位，计数和防连发延时全部放到主循环里做。

## 接线
- 传感器 DO（OUT）→ PB14
- VCC → 3.3V
- GND → GND
- OLED：SCL → PB8，SDA → PB9（显示用）

## 改造前后逐项对比

> 标注说明：**新增** = 改造时新写的；**修改** = 原有内容被改掉；**保留** = 没动；**删除** = 原有内容不再使用。
> 改造前版本取自首次提交 `a96f5ef`（跟教程直接抄的第一版），改造后版本即仓库当前代码。

### 1. `Hardware/CountSensor.c`

| 位置 | 类型 | 改造前 | 改造后 | 说明 |
|---|---|---|---|---|
| 头文件 | 删除 | `#include "Delay.h"` | 已去掉 | 中断里不再延时，这个模块就不该依赖 Delay；`Delay.h` 仍留在 `main.c` 里 |
| 全局变量 | 修改 + 新增 | `uint16_t CountSensor_Count;`（未显式初始化） | `uint16_t CountSensor_Count = 0;`；**新增** `volatile uint8_t Flag_Count = 0;` | `Flag_Count` 在中断置位、主循环清零，跨中断共享必须加 `volatile`，防止编译器把它优化成只读寄存器缓存 |
| EXTI 触发方式 | 修改 | `EXTI_Trigger_Rising_Falling`（双边沿） | `EXTI_Trigger_Falling`（只下降沿） | 遮挡（高→低）才计数；移开（低→高）不再进中断，一次遮挡不会被计成两次 |
| 中断服务函数 | 修改 | 两段 `Delay_ms(200)`，并在中断里直接 `CountSensor_Count++` | 确认 Line14 置位后，读 PB14 为低才 `Flag_Count = 1`，然后清中断标志 | 中断不再延时、不再计数，只负责"记下发生了" |
| 对外接口 | 新增 | 只有 `CountSensor_Get` | **新增** `CountSensor_Add` / `CountSensor_GetFlag` / `CountSensor_ClearFlag` | 计数动作和标志操作暴露给主循环调用 |
| `CountSensor_Get` | 保留 | `return CountSensor_Count;` | 原样 | 读累计值 |
| `CountSensor_Init` 其余部分 | 保留 | GPIOB/AFIO 时钟、PB14 上拉 IPU、`GPIO_EXTILineConfig`、NVIC（EXTI15_10_IRQn，抢占 1 子 1） | 原样 | 硬件初始化逻辑没变 |

### 2. `Hardware/CountSensor.h`

| 位置 | 类型 | 改造前 | 改造后 | 说明 |
|---|---|---|---|---|
| include guard | 修改 | `__COUNT_SENSOR_H` | `__COUNTSENSOR_H` | 改名，无功能影响 |
| 接口声明 | 新增 | 仅 `Init` / `Get` | **新增** `GetFlag` / `ClearFlag` / `Add` 声明 | 与 `.c` 里的新接口一一对应 |

### 3. `User/main.c`

| 位置 | 类型 | 改造前 | 改造后 | 说明 |
|---|---|---|---|---|
| 主循环 | 修改 | 循环里只做 `OLED_ShowNum` 刷新 | `if (GetFlag() == 1) { Add(); Delay_ms(200); ClearFlag(); }`，再刷新 OLED | 计数和 200ms 防连发从中断移到主循环；**先延时进死区、再清标志**，死区内新置的标志一并吞掉 |
| 头文件 | 保留 | `stm32f10x.h` / `Delay.h` / `OLED.h` / `CountSensor.h` | 原样 | 主循环仍要用 `Delay_ms` |

## 改造前完整代码（原版存档，未改动）

### `CountSensor.c`（原版）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"

uint16_t CountSensor_Count;

void CountSensor_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);

	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line14;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_Init(&EXTI_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
}

uint16_t CountSensor_Get(void)
{
	return CountSensor_Count;
}

void EXTI15_10_IRQHandler(void)
{
		if (EXTI_GetITStatus(EXTI_Line14) == SET)
		{
			Delay_ms(200);
			CountSensor_Count ++;
			Delay_ms(200);
			EXTI_ClearITPendingBit(EXTI_Line14);
		}
}
```

### `CountSensor.h`（原版）

```c
#ifndef __COUNT_SENSOR_H
#define __COUNT_SENSOR_H

void CountSensor_Init(void);
uint16_t CountSensor_Get(void);

#endif
```

### `main.c`（原版）

```c
#include "stm32f10x.h"                  // Device header
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
		OLED_ShowNum(1, 7, CountSensor_Get(), 5);
	}
}
```

## 改造后完整代码（仓库当前版本）

### `CountSensor.c`（现版）

```c
#include "stm32f10x.h"

uint16_t CountSensor_Count = 0;
volatile uint8_t Flag_Count = 0;   // 中断置位、主循环清零，跨中断共享必须 volatile

void CountSensor_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);

    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line14;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //只下降沿
    EXTI_Init(&EXTI_InitStructure);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
}

uint16_t CountSensor_Get(void)
{
    return CountSensor_Count;
}
// 计数值 +1，由主循环调用
void CountSensor_Add(void)
{
    CountSensor_Count ++;
}

uint8_t CountSensor_GetFlag(void)
{
	return Flag_Count;
}

void CountSensor_ClearFlag(void)
{
    Flag_Count = 0;
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line14) == SET)
    {
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0)
        {
            Flag_Count = 1;
        }
        EXTI_ClearITPendingBit(EXTI_Line14);
    }
}
```

### `CountSensor.h`（现版）

```c
#ifndef __COUNTSENSOR_H
#define __COUNTSENSOR_H

void CountSensor_Init(void);
uint16_t CountSensor_Get(void);
uint8_t CountSensor_GetFlag(void);
void CountSensor_ClearFlag(void);
void CountSensor_Add(void);  // 新增这一行

#endif
```

### `main.c`（现版）

```c
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
```

## 为什么要这么改（思路）

原版两个毛病：

1. **中断里塞了 400ms 延时**。中断服务函数应该尽量短。占着中断 400ms 期间 CPU 什么别的都干不了，OLED 不刷新，所有同优先级和低优先级的其他中断全被压着排队。
   - 补充：`Delay_ms` 是 SysTick 轮询 `COUNTFLAG` 实现的，不开启 SysTick 中断，所以原版**不会死锁**；真实代价是"占着中断 400ms、其他任务被压住"。
2. **双边沿触发会把一次遮挡计成两次**。遮挡时 DO 由高变低（下降沿，进中断），移开时由低变高（上升沿，又进一次中断）。只要两次之间超过了那 400ms 延时，一次遮挡就加了 2。

改完之后的职责分工：

| 谁 | 干什么 | 耗时 |
|---|---|---|
| 中断 `EXTI15_10_IRQHandler` | 只判断"有下降沿 + 引脚确实是低电平"，置 `Flag_Count = 1` | 几百纳秒 |
| 主循环 `while(1)` | 看到标志 → 加 1 → 延时 200ms 死区 → 清标志 → 刷新 OLED | 不占用中断 |

一句话：**中断只负责"记下发生了"，主循环负责"处理这件事"**。

## 遇到的问题

- **EXTI 必须开 AFIO 时钟**：`RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE)`，否则引脚到 EXTI 线的映射不生效，中断根本进不来。
- PB14 属于 Line14，对应的中断服务函数名是 `EXTI15_10_IRQHandler`（10~15 共用一个），不是单独的 EXTI14 中断函数；进了这个函数还要用 `EXTI_GetITStatus(EXTI_Line14)` 确认是哪根线触发的。
- 中断里那句 `GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0`，能挡掉"沿已经触发、但等中断跑起来时引脚已经弹回高电平"的极短毛刺，但它**不是严格意义的消抖**，别指望它彻底解决机械/光学抖动。
- 若计数一直为 0：先查传感器 DO 有没有接到 PB14、模块供电和指示灯是否正常，再查 AFIO 时钟、EXTI 线映射、NVIC 是否使能。

## 已改进记录（2026-11-22）

1. **死区顺序修正**：最初的主循环是 `Add() → ClearFlag() → Delay_ms(200)`，清标志在延时之前，200ms 死区并不生效——延时期间抖动产生的新下降沿会重新置位标志，延时结束后被再计一次。最终改为 `Add() → Delay_ms(200) → ClearFlag()`：先让抖动/重复遮挡过去，死区结束再清标志，这 200ms 内新置的标志一并吞掉。
2. **标志位加 `volatile`**：`Flag_Count` 在中断和主循环之间共享，加 `volatile` 防止编译器把它优化成只读寄存器缓存、读不到中断刚写入的值。

### 改完之后如果仍多计，按这个顺序排查

- 先确认是不是**一次物理遮挡产生了多个下降沿**：手在槽口前后晃动、物体边缘部分挡光，都可能多次触发。做对照测试：物体稳定放进槽口、保持不动、再拿出，看计数是否只 +1。
- 200ms 死区偏短可适当加大（如 300~500ms），代价是连续快速遮挡时会被漏记。
- 硬件上可在 DO 引脚到 GND 并 100nF 滤波电容；纯软件中断方式对机械/光学抖动的抑制到此为止，要更稳需要改成定时器扫描 + 状态机解码。
