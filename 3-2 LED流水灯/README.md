# 3-2 LED 流水灯

## 功能
PA0~PA7 八个 LED 依次逐个点亮，循环流水。每个状态保持 500ms。

## 接线
- LED1~LED8 → PA0、PA1、PA2、PA3、PA4、PA5、PA6、PA7
- VCC → 3.3V
- GND → GND

## 用到的代码
- `System/Delay.c/.h`：`Delay_ms` 延时
- `User/main.c`：开 GPIOA 时钟 → PA 全部引脚配推挽输出 → while 循环用 `GPIO_Write(GPIOA, 位掩码)` 整端口写电平
- 关键写法：`GPIO_Write(GPIOA, ~0x0001)`、`~0x0002` … `~0x0080`，每次只留一位为低（点亮），其余为高（熄灭）

## 遇到的问题
- 和 3-1 单个引脚操作不同，这里用 `GPIO_Write` 一次性写整个端口的 16 位 ODR 寄存器，不是 `GPIO_WriteBit` 那种只改一位。
- 代码里掩码前面有 `~`（取反）：因为套件 LED 低电平点亮，想点亮哪一位就把那一位写成 0，所以对 `0x0001` 这种位掩码取反后再写。如果板子是高电平点亮，去掉 `~` 即可。
- `GPIO_Pin_All` 把 PA 全部引脚都配成输出；实际流水只用到 PA0~PA7，多余引脚不影响。
- 延时仍是 `Delay_ms` 空转，流水速度想调快/调慢就改延时数值。
