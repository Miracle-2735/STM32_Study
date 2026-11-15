# 3-4 按键控制 LED

## 功能
按键 1 按下 LED1 翻转（亮↔灭），按键 2 按下 LED2 翻转。主循环不断读按键，读到按键编号后调用对应 LED 的翻转函数。

## 接线
- LED1 → PA1
- LED2 → PA2
- Key1 → PB1
- Key2 → PB11
- VCC → 3.3V
- GND → GND

## 用到的代码
- `System/Delay.c/.h`：`Delay_ms`，按键消抖用
- `Hardware/LED.c/.h`：`LED_Init`、`LED1_ON/OFF/Turn`、`LED2_ON/OFF/Turn`；PA1、PA2 推挽输出，初始化先拉高（熄灭）
- `Hardware/Key.c/.h`：`Key_Init`、`Key_GetNum`；PB1、PB11 配**上拉输入 IPU**，按下接 GND 电平变低，读到低后延时 20ms 消抖、等松手、再延时 20ms，返回按键编号 1 或 2
- `User/main.c`：初始化 LED 和按键，while 循环里 `KeyNum = Key_GetNum()`，按 1 翻 LED1，按 2 翻 LED2

## 遇到的问题
- **Key.c 漏写 `#include "Delay.h"`**：`Delay_ms` 未声明，编译出黄色警告。补上头文件后警告消失。
- 按键用**上拉输入**：默认被内部上拉拉成高电平，按键另一端接 GND，按下才拉低；所以判断条件是 `== 0`（低电平有效），不要写成 `== 1`。
- 机械按键按下/松开瞬间会抖动，直接读会一次按下被识别成多次，代码里用两段 `Delay_ms(20)` 做软件消抖。
- LED 仍是低电平点亮：`Turn` 里读输出寄存器当前电平再取反。
