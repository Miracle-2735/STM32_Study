# 5-2 旋转编码器计次

## 功能
EC11 旋转编码器正转计数加、反转计数减，OLED 用有符号数显示累计值。A、B 两相都接 EXTI 中断、**只下降沿触发**，在 A 相中断里读 B 相电平判断旋转方向（B 相中断里读 A 相）。

## 接线
- A 相（CLK）→ PB0
- B 相（DT）→ PB1
- VCC → 3.3V
- GND → GND
- OLED：SCL → PB8，SDA → PB9（显示用）

## 用到的代码
- `System/Delay.c/.h`：延时（工程保留）
- `Hardware/Encoder.c/.h`：`Encoder_Init`、`Encoder_Get`
  - 开 GPIOB、AFIO 时钟，PB0、PB1 配**上拉输入 IPU**
  - `GPIO_EXTILineConfig` 把 PB0→Line0、PB1→Line1
  - EXTI Line0 | Line1，**只下降沿**触发（`EXTI_Trigger_Falling`），中断模式
  - NVIC：`EXTI0_IRQn`（抢占 1、子 1）、`EXTI1_IRQn`（抢占 1、子 2）
  - `EXTI0_IRQHandler`：进 A 相中断，若 PB1 为低则 `Encoder_Count--`
  - `EXTI1_IRQHandler`：进 B 相中断，若 PB0 为低则 `Encoder_Count++`
  - `Encoder_Get`：先把 `Encoder_Count` 存到临时变量，再清零，返回临时值（主循环取走一次就清零，避免同一格被重复累加）
- `User/main.c`：`Num += Encoder_Get()`，`OLED_ShowSignedNum` 显示带符号结果

## 11-22 改动：双边沿 → 只下降沿

原来两相都配 `EXTI_Trigger_Rising_Falling`，现在只配 `EXTI_Trigger_Falling`，中断服务函数一个字没动。这一行改动的依据是数一下"转一格会进几次中断"。

正转（A 相领先 B 相 90°）：

| 事件 | 引脚电平 | 是否触发中断 | 中断里的判断 | 结果 |
|---|---|---|---|---|
| A 相下降沿 | A 低、**B 还是高** | 触发（EXTI0） | `if (PB1 == 0)` 不成立 | 不计数 |
| B 相下降沿 | B 低、**A 已经低** | 触发（EXTI1） | `if (PB0 == 0)` 成立 | +1 |

反转（B 相领先 A 相 90°）：A 下降沿时 B 已经变低 → EXTI0 里 `PB1 == 0` 成立 → −1；B 下降沿时 A 还是高 → EXTI1 不成立 → 不加。

所以**只留下降沿，转一格恰好 ±1**，两相的上升沿本来就是多余的那一半。原来双边沿时，一次转格会进 4 次中断（2 个下降沿 + 2 个上升沿），其中两次判断成立、两次不成立，方向虽然不会错，但**触点抖动的机会翻倍**——上升沿那两次抖动，也会带着错误的方向判断一起进来，这就是"转一格忽加忽减"的来源之一。

## 遇到的问题（本实验踩坑最集中，按排查顺序记）

### 1. 代码笔误：EXTI 线写重复
- 错误：`EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line0;`
- 正确：`EXTI_Line0 | EXTI_Line1`。两相要分别对应 Line0 和 Line1，写成两个 Line0 等于 B 相中断没配上。

### 2. 清标志位置错位置
- `EXTI_ClearITPendingBit` 要放在 `if (EXTI_GetITStatus(...) == SET)` **里面**；写在 if 外面会在没进中断时也清标志，逻辑不对，删掉多余的那处。

### 3. 编译报 `cannot open source input file "Encoder.h"`
- 原因：`Encoder.h` 所在文件夹没加进 Keil 的 Include Paths，或文件放错目录。
- 处理：Options for Target → C/C++ → Include Paths 加上 Hardware 文件夹；确认 .h 和 .c 在同一目录、文件名大小写一致。

### 4. 加 `Encoder_Get` 时把中断函数覆盖丢了
- 改动过程中 `EXTI0_IRQHandler` 被覆盖/删掉，导致 A 相中断不进。发现后要把两个中断服务函数都补回来，别只留 `Encoder_Get`。

### 5. 计数一直不变，分步定位
按从硬件到软件的顺序一层层排除：
1. **先验证引脚物理连通和 GPIO 配置**：在 main.c 临时直接读引脚 `Num = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)`，把 PB0 碰 3.3V 显示 1、碰 GND 显示 0，说明线通、GPIO 输入配置正确。
2. **再验证 EXTI 中断和 NVIC**：把 `EXTI0_IRQHandler` 临时改成无条件 `Encoder_Count++`（去掉方向判断），手动碰 PB0，数字从 0 涨到 86，说明 EXTI 中断硬件和 NVIC 配置正常。
3. **最初计数不变的真正原因**：方向判断条件 `if (PB1 == 0)` 在手动测试时只碰了 PB0、PB1 没动（保持高电平），条件不满足，所以不加也不减。这不是中断没进，是方向判断没通过。

### 6. 恢复方向判断后，转一格数字忽加忽减（+1 / +2 / +18 不等）
- 定位：EC11 是机械触点编码器，A/B 相通断瞬间有触点抖动，江科大标准库这套代码**没有软件消抖**，抖动会被中断当成多个跳变沿，于是出现多计、跳变方向乱跳。
- 11-22 已把双边沿改成**只下降沿**，等于把一次转格进中断的次数从 4 次减到 2 次，抖动被误判的概率约减半，但**没有根治**——`+1 / +2 / +18` 这种大幅跳变仍可能出现。
- 结论：这是机械抖动通病，**不是板子或编码器质量问题**；套件（江科大推荐套）同款都有这个现象。彻底解决要靠：硬件 RC 滤波（A/B 各串 10k 上拉 + 100nF 到地），或软件上改成定时器 1ms 扫描 + 状态机解码（比中断更稳，也是实际项目常用做法）。

### 7. 方向判断原理备查
- 正转/反转时 A、B 两相方波的相位先后不同：A 相跳变瞬间，B 相是高还是低，决定这一格算正转还是反转。代码里就是靠"在一相中断里读另一相电平"来判方向。
