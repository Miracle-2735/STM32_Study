# 6-2 定时器外部时钟

## 功能
TIM2 工作在**外部时钟模式 2**，PA0（TIM2_ETR）接对射红外传感器，传感器每给一个脉冲，硬件计数器 CNT 加 1。CNT 数到 ARR（9）后硬件自动回 0，同时触发更新中断，中断里软件变量 `Num++` 统计溢出次数。OLED 第一行显示 Num（溢出次数），第二行显示 CNT（当前计数值）。

## 接线
- 对射红外 DO（脉冲输出）→ PA0（TIM2_ETR）
- VCC → 3.3V
- GND → GND
- OLED：SCL → PB8，SDA → PB9（显示用）

## 用到的代码
- `System/Timer.c/.h`：`Timer_Init`、`Timer_GetCounter`、中断函数 `TIM2_IRQHandler`
  - 开 TIM2（APB1）和 GPIOA（APB2）时钟
  - PA0 配**上拉输入 IPU**，接 ETR 外部脉冲
  - `TIM_ETRClockMode2Config(TIM2, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0x00)`：外部时钟模式 2，不归滤波、不反相
  - 时基：`TIM_Period = 10 - 1`（ARR=9）、`TIM_Prescaler = 1 - 1`（PSC=0），向上计数
  - 清更新标志、开更新中断、配 NVIC（TIM2_IRQn，抢占 2、子 1）、`TIM_Cmd` 启动
  - `Timer_GetCounter()` 返回 `TIM_GetCounter(TIM2)` 读当前 CNT
  - 中断里判断更新中断置位 → `Num++` → 清标志
- `User/main.c`：定义全局 `uint16_t Num`，初始化 OLED 和定时器，while 循环显示 Num 和 `Timer_GetCounter()`

## 先分清 Num 和 CNT（容易搞混，重点看）

| 变量 | 是什么 | 谁来改 | 说明 |
|---|---|---|---|
| CNT（`TIM_GetCounter`） | 硬件计数器当前值 | 定时器硬件自动改 | ETR 来一个脉冲 +1，数到 ARR(9) 后**硬件自动回 0**，不需要软件管 |
| Num | 软件统计的溢出次数 | 中断里手动 `Num++` | CNT 每溢出（回 0）一次，进一次更新中断，Num 加 1 |

- 流程顺序：**CNT 数到 9 → 硬件回 0 并置更新标志 → 进更新中断 → 中断里 Num++ → 清标志**。
- CNT 重置是硬件行为，是"因"；Num++ 是中断里做的软件统计，是"果"。**不是 Num++ 推动 CNT 走到 9 再重置。**

### 为什么教程没写 Num 加加，CNT 也能走到 9 再回 0？
- 因为"数到 9 回 0"是定时器硬件按 ARR 自动做的，只要定时器使能、ETR 有脉冲进来就发生，和软件写不写 `Num++` 没有任何关系。
- 教程如果 OLED 只显示 CNT，本来就不需要 Num；Num 是自己额外加来统计"已经溢出了多少次"的。加不加 Num，CNT 该怎么数还怎么数。
- 觉得"只有 Num 增加 CNT 才走到 9"是错觉：CNT 在前面自己数到顶，Num 只是后面跟着记一笔。

### 为什么自己现象和教程不一样？
按下面顺序排查，绝大多数是软件/工程配置问题，不是定时器坏了：
1. 中断服务函数是不是被注释掉了（见问题 1）；
2. `Num` 跨文件有没有加 `extern`（见问题 2）；
3. `Timer.c` 有没有被编译进工程（见问题 6）；
4. 传感器脉冲到底有没有进 PA0——外部时钟模式下，没有新脉冲 CNT 就停在当前值不动；
5. 接线接触、传感器抖动、遮挡手法（见问题 5）。

## 遇到的问题

### 1. 中断服务函数被 `/* */` 整体注释，且漏写 `Num++`
- 最初 `TIM2_IRQHandler` 被第 44 行 `/*` 到第 53 行 `*/` 整段包住注释，中断根本不进，Num 永远是 0。
- 即使取消注释，大括号里还漏了 `Num++`，只有清标志、没有累加。
- 正确写法：
```c
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        Num++;                                    // 溢出一次，Num 加 1
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
```

### 2. `Num` 跨文件使用要加 `extern`
- `Num` 定义在 main.c，Timer.c 的中断里要用它。Timer.c 顶部必须声明 `extern uint16_t Num;`，否则编译/链接找不到这个变量。

### 3. 显示时只写函数名、没加括号参数，输出垃圾大数
- 直接把函数名（函数地址）丢给显示/打印函数，会输出一个很大的随机数（如 1563693376），因为函数名代表的是地址不是返回值。
- 必须写成带括号和参数的调用，例如 `Timer_GetCounter()`、`countPass(s, 5)`。

### 4. CNT 在 5、7 附近晃、不动
- 外部时钟模式下 CNT 靠 ETR 脉冲 +1，手松开、不再遮挡就没有新脉冲，CNT 停在当前值等下一次，这是正常的，不是定时器卡住。
- 数值在几个数之间小幅晃动，和对射红外机械抖动、遮挡手法快慢、杜邦线接触有关，和 5-2 编码器抖动是同类问题。

### 5. 编译警告 `#1-D: last line of file ends without a newline`
- Build Output 显示 `0 Error(s), 1 Warning(s)`，这个警告只是 main.c 最后一行末尾缺一个换行符，**不影响运行**，可不管；想消除就在文件末尾补一空行。

### 6. 确认 `.c` 文件真的被编译进工程
- 在 Keil 工程窗口里，源文件图标若是**灰色半透明**，说明被排除在 Target Build 之外（Include in Target Build 未勾选），改了代码也不会参与编译。右键文件 → Options → 勾上 Include in Target Build。
- 改完代码要重新 Build，确认 Output 里确实编译了 `Timer.c`。
