# 2-1 STM32 工程模板

## 功能
搭好一个可以直接编译下载的标准库（SPL）工程骨架，后面每个实验都从这个模板复制一份再改代码。当前 main.c 里只做了一件事：把 PC13 配成推挽输出并拉低，用来点灯验证模板本身能跑通。

## 接线
- PC13 → 板载 LED（开发板自带，一般不用外接杜邦线）
- 其余外设暂时不接

## 用到的代码
- `Library/`：STM32 标准库源码（gpio/rcc/exti/tim/usart 等 .c/.h）
- `Start/`：启动文件 `startup_stm32f10x_md.s`、`stm32f10x.h`、`system_stm32f10x.c`
- `User/main.c`：主函数，模板里做 PC13 推挽输出测试
- `User/stm32f10x_it.c/.h`：中断服务函数文件（默认空）
- `User/stm32f10x_conf.h`：标准库头文件总包含
- `Project.uvprojx`：Keil5 工程文件，双击用 MDK 打开

## 遇到的问题
- 模板目录要固定：Library / Start / User 分开放，后面自己加的外设驱动（LED、Key、OLED 等）统一放新建的 `Hardware/` 文件夹，延时放 `System/`。
- 新建 .c/.h 后，如果编译提示找不到头文件（`cannot open source input file "xxx.h"`），要在 Keil → Options for Target → C/C++ → Include Paths 里把对应文件夹加进去，并确认该 .c 文件已加入工程编译（工程窗口里文件若是灰色半透明，说明被排除在编译之外）。
- 启动文件选 `md`（中容量产品，STM32F103C8T6），不要选错成 cl/hd/xl。
