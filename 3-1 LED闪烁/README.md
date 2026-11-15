# 3-1 LED 闪烁

## 功能
PA0 接的 LED 以 500ms 间隔亮灭闪烁。main 里用三种等价写法各写了一轮点灯/熄灭，对比 GPIO 输出的几种 API：
- `GPIO_ResetBits` / `GPIO_SetBits`
- `GPIO_WriteBit(..., Bit_RESET / Bit_SET)`
- `GPIO_WriteBit(..., (BitAction)0 / (BitAction)1)`

## 接线
- LED → PA0
- VCC → 3.3V
- GND → GND

## 用到的代码
- `System/Delay.c/.h`：`Delay_ms` 毫秒延时
- `User/main.c`：开 GPIOA 时钟 → 配 PA0 推挽输出（Out_PP，50MHz）→ while 循环里轮流拉低/拉高
- 关键配置：`GPIO_Mode = GPIO_Mode_Out_PP`（推挽输出，主动出高低电平，驱动 LED）

## 遇到的问题
- 这是第一个 GPIO 输出实验，主要卡点在配置顺序：必须先 `RCC_APB2PeriphClockCmd` 开 GPIOA 时钟，再 `GPIO_Init` 配引脚；不开时钟引脚不工作。
- 套件 LED 是低电平点亮：`ResetBits`（拉低）亮，`SetBits`（拉高）灭，和直觉相反，先确认自己板子是高电平还是低电平点亮。
- `(BitAction)0` / `(BitAction)1` 是把整数强转成枚举类型，和直接写 `Bit_RESET`/`Bit_SET` 效果一样。
- 延时靠 `Delay_ms` 空转，不是定时器定时，精度一般，只够做闪烁演示。
