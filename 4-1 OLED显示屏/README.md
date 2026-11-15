# 4-1 OLED 显示屏

## 功能
0.96 寸 OLED（SSD1306，软件 I2C 驱动）在屏幕上显示字符、字符串、十进制数、有符号数、十六进制数和二进制数，验证 OLED 各显示函数。

## 接线
- OLED SCL → PB8
- OLED SDA → PB9
- OLED VCC → 3.3V
- OLED GND → GND

## 用到的代码
- `System/Delay.c/.h`：延时（I2C 时序里用）
- `Hardware/OLED.c/.h`：`OLED_Init`、`OLED_ShowChar`、`OLED_ShowString`、`OLED_ShowNum`、`OLED_ShowSignedNum`、`OLED_ShowHexNum`、`OLED_ShowBinNum` 等；底层用软件模拟 I2C 时序
- `Hardware/OLED_Font.h`：ASCII 字模表
- `User/main.c`：`OLED_Init()` 后依次在第 1~4 行显示字符、字符串、数字、有符号数、十六进制、二进制

## 遇到的问题
- SCL/SDA 两个引脚配的是**开漏输出 Out_OD**（不是推挽），I2C 总线靠开漏 + 上拉电阻线与，这是 I2C 引脚的标准配法，别照 LED 那样配成推挽。
- 软件 I2C 是用 GPIO 手动翻转电平模拟时序，没有用片上 I2C 外设；改引脚要同时改 `OLED.c` 顶部 `OLED_W_SCL` / `OLED_W_SDA` 两个宏定义。
- 显示函数第一个参数是行号（1~4），第二个是列号（1~16），数字显示最后一个参数是位数；位数给小了高位会被截断。
- 屏幕不亮/花屏先查：VCC 是不是 3.3V、SCL/SDA 有没有接反、模块对比度/初始化是否跑到位。
