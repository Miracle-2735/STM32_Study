# 6-5 PWM 驱动直流电机

## 功能
TIM2_CH3 输出 PWM 给电机驱动板调速，PA4/PA5 两根普通 GPIO 做方向控制（接 H 桥输入）；按 Key1 速度 +20，超过 100 回到 -100（从正转满速切到反转满速循环），OLED 用带符号数字显示当前速度。

## 接线
- PA2（TIM2_CH3）→ 驱动板 PWM 调速输入
- PA4、PA5 → 驱动板方向控制 IN1/IN2（推挽输出）
- 直流电机 → 驱动板输出端（**STM32 不直接驱动电机**）
- 驱动板/电机单独供电，与 STM32 共地 GND
- 按键：Key1 → PB1，Key2 → PB11
- OLED：SCL → PB8，SDA → PB9

## 用到的代码
- `Hardware/PWM.c/.h`：`PWM_Init`、`PWM_Setcompare3`
  - 开 TIM2、GPIOA 时钟；PA2 配 AF_PP
  - 时基：`TIM_Period = 100 - 1`（ARR=99）、`TIM_Prescaler = 36 - 1`（PSC=35）
    - 计数时钟 72MHz / 36 = 2MHz；周期 100 个计数 = 50μs，频率 **20kHz**（避开可闻啸叫）
  - `TIM_OCMode_PWM1`、High、Enable，`TIM_OC3Init`，`TIM_Cmd` 启动
  - `PWM_Setcompare3` → `TIM_SetCompare3(TIM2, Compare)`
- `Hardware/Motor.c/.h`：`Motor_Init`、`Motor_SetSpeed(int8_t Speed)`
  - PA4、PA5 配推挽输出
  - `Speed >= 0`：PA4=1、PA5=0（正转），PWM = Speed
  - `Speed < 0`：PA4=0、PA5=1（反转），PWM = -Speed
- `Hardware/Key.c/.h`：按键消抖读取
- `User/main.c`：初始化 OLED/电机/按键；Key1 按下 Speed+=20，>100 置 -100；`Motor_SetSpeed(Speed)`，`OLED_ShowSignedNum` 显示带符号速度

## 遇到的问题

### 1. 链接报错 L6218E：Undefined symbol `PWM_Setcompare3`
- Build Output：`Error: L6218E: Undefined symbol PWM_Setcompare3 (referred from motor.o)`。
- 原因：从 6-3（CH1）复制代码后，内部 `TIM_OC3Init`、`TIM_SetCompare3` 都改成了通道 3，但 PWM.c 里封装函数名还留着 `PWM_Setcompare1`；而 PWM.h 声明、Motor.c 调用用的都是 `PWM_Setcompare3`，链接时按名字找不到函数体。
- 处理：把 PWM.c 里定义改成 `void PWM_Setcompare3(uint16_t Compare)`，三处（.c 定义 / .h 声明 / Motor.c 调用）名字一致，编译通过。
- 教训：复制别的通道代码后，函数名、OCxInit、SetComparex、GPIO Pin 要一起核对改全。

### 2. 电机裸铜片接线拧变形
- 130 小电机出厂是两片裸铜焊片，标准接法是**把导线焊到铜片上**；用杜邦端子/夹子硬拧，铜片很薄，受力就被掰翘变形。
- 临时验证：鳄鱼夹分别夹住两片铜片接驱动板输出，只能短时静态试，振动下容易松脱打火；长期用必须焊线，焊完两片之间、铜片与外壳套热缩管/缠胶布绝缘。
- 若铜片根部已经开裂、能晃动或缩进电机壳，补焊容易反复，这种电机几块钱一个，直接换更划算。

### 3. 电机不能用 GPIO 直接驱动
- 电机是电流负载，STM32 引脚电流带不动，必须经过 H 桥/电机驱动板：GPIO 只给方向电平，PWM 给调速占空比，电机接驱动板输出。
- 方向由 PA4/PA5 电平组合决定，转速由 PWM 占空比决定；驱动板和电机要单独供电并共地。

### 4. 其他
- 局部变量名早期误拼为 `Steucture`，本工程代码里已是正确的 `TIM_OCInitStructure`（6-3/6-4 也已于 2026-11-22 一并修正），不影响功能。
- `Motor_SetSpeed` 参数是带符号 int8_t：正数正转、负数反转，传进 PWM 比较值时取 `-Speed`。