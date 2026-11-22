# 6-4 PWM 驱动舵机

## 功能
TIM2_CH2 输出 50Hz 舵机控制信号，按角度换算成对应脉宽；按 Key1 每次角度 +30°（超过 180° 回 0），OLED 显示当前角度。

## 接线
- PA1（TIM2_CH2）→ 舵机信号线（橙/黄线）
- 舵机 VCC → 外部 5V（舵机电流大，不要从 STM32 3.3V 引脚取动力电）
- 舵机 GND → 与 STM32 **共地**
- 按键：Key1 → PB1，Key2 → PB11（上拉输入，低电平有效）
- OLED：SCL → PB8，SDA → PB9

## 用到的代码
- `Hardware/PWM.c/.h`：`PWM_Init`、`PWM_Setcompare2`
  - 开 TIM2、GPIOA 时钟；PA1 配 AF_PP
  - 时基：`TIM_Period = 20000 - 1`（ARR=19999）、`TIM_Prescaler = 72 - 1`（PSC=71）
    - 计数时钟 72MHz / 72 = 1MHz（1 个计数 = 1μs）；周期 20000μs = 20ms = **50Hz**，舵机标准频率
  - `TIM_OCMode_PWM1`、High、Enable，`TIM_OC2Init`，`TIM_Cmd` 启动
  - `PWM_Setcompare2` → `TIM_SetCompare2(TIM2, Compare)`
- `Hardware/Servo.c/.h`：`Servo_Init`（调 PWM_Init）、`Servo_SetAngle(float Angle)`
  - 角度换算：`CCR = Angle / 180 * 2000 + 500`
  - 0° → 500μs，180° → 2500μs（1MHz 计数下 CCR 数值即微秒数）
- `Hardware/Key.c/.h`：`Key_Init`、`Key_GetNum`，PB1/PB11 上拉输入，20ms 软件消抖
- `User/main.c`：初始化 OLED/舵机/按键，初始 90°；Key1 按下 Angle+=30，>180 回 0；`Servo_SetAngle(Angle)`，OLED 显示角度

## 遇到的问题 / 注意事项

### 1. 舵机频率不能照搬呼吸灯参数
- 舵机要求 **50Hz（20ms 周期）**，ARR/PSC 必须按 20ms 配；直接套用 6-3 的 1kHz 参数，舵机不会正常转。
- 脉宽范围 0.5\~2.5ms 对应 0\~180°，CCR 超出这个范围舵机可能抖动、堵转或乱动；具体脉宽范围以所用舵机 datasheet 为准，必要时校准公式。

### 2. 供电和共地
- 舵机转动电流比 GPIO 大，动力电要单独供（外部 5V/驱动板），不要指望 STM32 3.3V 引脚带；信号线是 3.3V 电平，舵机一般能识别。
- 舵机 GND 必须和 STM32 共地，否则信号参考电平不稳，舵机乱抖。

### 3. 其他
- 局部变量名 `Steucture` 拼写不规范。**已于 2026-11-22 统一改名为 `TIM_OCInitStructure`**（与 6-3 一并修正），纯改名、逻辑不变。
- 本工程未单独用逻辑分析仪抓波形，参数以上述源码计算为准。