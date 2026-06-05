# 硬件接线与 CubeMX 配置档案

本文档记录信号分离装置的硬件引脚分配和 CubeMX 配置方案，内容已与当前仓库代码（`.ioc`、`main.h`、各驱动模块）对齐。

## 当前仓库状态

- `xiaosai.ioc` 和 `Core/Inc/main.h` 已生成 AD7616、AD9833、AD9834、LCD 四组引脚宏
- `HARDWARE/INC` 中各驱动头文件保留了 `#ifndef` 兜底定义，防止 CubeMX 重新生成导致编译失败
- 软件链路已落地：GPIO 并口采集 AD7616 → CMSIS-DSP FFT → 场景识别 → AD9833/AD9834 双路 DDS 输出 → ST7735S LCD 显示

---

## 引脚分配原则

F407 板的排针引出分为 P4 和 P5 两组，引脚分配按以下原则确定：

1. AD7616 的 16 位数据总线放置于 PD0~PD15，因这些引脚均位于 P5 排针，适合整组排线连接
2. LCD 六根信号线放置于 PB10~PB15，均位于 P4 底部区域，便于成组接线
3. 两片 DDS 各自使用独立 SPI 总线——AD9833 占用 GPIOA（PA2/PA4/PA6），AD9834 占用 GPIOE（PE2~PE6）+ GPIOC（PC0），互不共享

### 已占用引脚

以下引脚已在工程中使用，不可分配给新外设：

| 用途 | STM32 引脚 | 备注 |
|---|---|---|
| HSE 晶振输入 | PH0 | 25MHz |
| HSE 晶振输出 | PH1 | 25MHz |
| 蓝灯 LED_BULL | PB2 | 运行指示 |
| 红灯 LEC_RED | PC5 | 错误指示 |
| SWDIO | PA13 | 调试接口 |
| SWCLK | PA14 | 调试接口 |

CubeMX 中 Debug 选项选择 `Serial Wire`。选择 `Disable` 将导致调试器无法连接；选择 `Full SWJ` 会额外占用 PB3/PB4，可能与其余外设冲突。

PA0 最初配置为按键触发采样，当前版本已改为上电自动循环，PA0 保留为空闲状态。

---

## 引脚分配表

### AD7616 — 16 位外部 ADC，并口模式

数据总线使用 PD0~PD15（全部位于 P5 排针），控制线主要使用 PC6~PC12，少量使用 PA8~PA11 和 PA15。驱动使用 GPIO 软件翻转 CONV 和 RD 引脚。

| AD7616 信号 | STM32 引脚 | 模式 | CubeMX Label | 说明 |
|---|---|---|---|---|
| DB0~DB15 | PD0~PD15 | GPIO_Input | AD7616_D0~D15 | 并口数据位，按位序连接 |
| CS | PC6 | GPIO_Output | AD7616_CS | 片选（低有效） |
| RD | PC7 | GPIO_Output | AD7616_RD | 读使能（低有效） |
| WR | PC8 | GPIO_Output | AD7616_WR | 写使能（低有效） |
| RST | PC9 | GPIO_Output | AD7616_RST | 复位（低有效） |
| RNG0 | PC10 | GPIO_Output | AD7616_RNG0 | 量程选择 bit0 |
| RNG1 | PC11 | GPIO_Output | AD7616_RNG1 | 量程选择 bit1 |
| CHS0 | PC12 | GPIO_Output | AD7616_CHS0 | 通道选择 bit0 |
| CHS1 | PA8 | GPIO_Output | AD7616_CHS1 | 通道选择 bit1 |
| CHS2 | PA9 | GPIO_Output | AD7616_CHS2 | 通道选择 bit2 |
| SER/PAR | PA10 | GPIO_Output | AD7616_S_P | 模式选择，低电平为并口 |
| BUSY | PA11 | GPIO_Input | AD7616_BUSY | 转换忙信号，不可悬空 |
| CONVST | PA15 | GPIO_Output | AD7616_CONV | 启动转换脉冲 |

PD0~PD15 和 PC6~PC12 均位于 P5 排针，数据总线和大部分控制线可在同一排针区域完成接线。

### AD9833 — DDS 低频通道

P2 模块引出 FSYNC/SCK/SDATA 三根信号线（FS/PS/RST/MCLK 由板载电路处理），使用 GPIO 软件模拟 SPI 时序。主钟 25MHz，最高输出频率 12.5MHz。

| AD9833 P2 引脚 | STM32 引脚 | 模式 | CubeMX Label |
|---|---|---|---|
| FSYNC (pin2) | PA2 | GPIO_Output | AD9833_FSYNC |
| SCK (pin4) | PA4 | GPIO_Output | AD9833_SCLK |
| SDATA (pin6) | PA6 | GPIO_Output | AD9833_SDATA |
| GND (pin1/3/5) | GND | — | — |
| VDD | 3.3V/5V | — | — |

信号集中在 P4 的 PA 区域。

### AD9834 — DDS 高频通道

P4 模块引出 FSY/SCK/SDA/RST/FS/PS 六根信号线，FS 和 PS 引脚各串联 100R 限流电阻。使用 GPIO 软件模拟 SPI 时序。主钟 75MHz，最高输出频率 37.5MHz。

| AD9834 P4 引脚 | STM32 引脚 | 模式 | CubeMX Label |
|---|---|---|---|
| FSY (pin1) | PE2 | GPIO_Output | AD9834_FSY |
| SCK (pin3) | PE4 | GPIO_Output | AD9834_SCK |
| SDA (pin5) | PE6 | GPIO_Output | AD9834_SDA |
| RST (pin7) | PC0 | GPIO_Output | AD9834_RST |
| FS (pin2, 100R) | PE3 | GPIO_Output | AD9834_FS |
| PS (pin4, 100R) | PE5 | GPIO_Output | AD9834_PS |
| VDD (pin6/8) | 5V | — | — |
| GND (pin9/10) | GND | — | — |

PE2~PE6 全部位于 GPIOE，仅 RST 位于 GPIOC（PC0）。

### ST7735S LCD

使用 GPIO 模拟串行时序（非硬件 SPI），六根信号线集中分布于 PB10~PB15。

| LCD 信号 | STM32 引脚 | 模式 | CubeMX Label |
|---|---|---|---|
| SCL | PB13 | GPIO_Output | LCD_SCL |
| SDA | PB15 | GPIO_Output | LCD_SDA |
| CS | PB12 | GPIO_Output | LCD_CS |
| DC | PB14 | GPIO_Output | LCD_DC |
| RES | PB10 | GPIO_Output | LCD_RES |
| BLK | PB11 | GPIO_Output | LCD_BLK |

`lcd.h` 通过 `#if defined()` 链同时兼容 `LCD_SCL` 系列和 `SCL` 系列两套命名。当前 CubeMX 中统一填写 `LCD_*` 前缀。

---

## 引脚冲突检查

1. PA8 分配给 AD7616_CHS1，与 LCD 无冲突（LCD 使用 PB10~PB15）
2. PC5 保留给红灯 LEC_RED，与 AD7616 控制线无冲突
3. PB10~PB15 整组分配给 LCD，无其他外设占用
4. PA15 独占 AD7616_CONV，CubeMX 中配置为 GPIO_Output
5. PD0~PD15 整组分配给 AD7616 数据总线
6. AD9833（PA2/PA4/PA6）与 AD9834（PE2~PE6 + PC0）无共享引脚，且与 P5 数据总线无冲突
7. PA0（KEY）与 AD9833 的 PA2/PA4/PA6 无冲突（PA1/PA3/PA5 为间隔引脚）
8. GPIOE 中 PE2~PE6 被 AD9834 占用，PE7~PE15 空闲可用

---

## CubeMX 配置

### 时钟树

```
HSE = 25MHz（板载晶振）
PLLM = 25, PLLN = 336, PLLP = 2  →  SYSCLK = 168MHz
HCLK = 168MHz
APB1 = 42MHz（÷4）
APB2 = 84MHz（÷2）
TIM2 挂载于 APB1 → Timer Clock = 84MHz（APB1 定时器时钟 = APB1 ×2）
```

### Debug 接口

`System Core → SYS → Debug = Serial Wire`

仅使用 PA13（SWDIO）和 PA14（SWCLK），释放 PA15 用于 AD7616_CONV。

### GPIO 配置

以下信号统一设为 `GPIO_Output`，输出类型 `Push Pull`，上下拉 `No Pull`，速度 `Very High`：
- AD9833 全部 GPIO（PA2/PA4/PA6）
- AD9834 全部 GPIO（PE2~PE6 + PC0）
- AD7616 控制脚（CS/RD/WR/RST/CHS0~2/RNG0~1/S_P/CONV）
- LCD 全部 GPIO（SCL/SDA/CS/DC/RES/BLK）

以下信号设为 `GPIO_Input`，`No Pull`：
- AD7616 BUSY
- AD7616 D0~D15

### CubeMX Label

CubeMX 中填入的 GPIO Label 需与驱动代码宏名对应。最终使用的 Label 如下：

- PD0~PD15: `AD7616_D0` ~ `AD7616_D15`
- PC6~PC12: `AD7616_CS`, `AD7616_RD`, `AD7616_WR`, `AD7616_RST`, `AD7616_RNG0`, `AD7616_RNG1`, `AD7616_CHS0`
- PA8~PA11, PA15: `AD7616_CHS1`, `AD7616_CHS2`, `AD7616_S_P`, `AD7616_BUSY`, `AD7616_CONV`
- PA2/PA4/PA6: `AD9833_FSYNC`, `AD9833_SCLK`, `AD9833_SDATA`
- PE2~PE6: `AD9834_FSY`, `AD9834_FS`, `AD9834_SCK`, `AD9834_PS`, `AD9834_SDA`
- PC0: `AD9834_RST`
- PB10~PB15: `LCD_RES`, `LCD_BLK`, `LCD_CS`, `LCD_SCL`, `LCD_DC`, `LCD_SDA`

---

## 采样方案演进

### 第一阶段：GPIO 软件翻转 CONV

初始驱动实现为 GPIO 软件脉冲：

```c
AD7616_Conversion() {
    HAL_GPIO_WritePin(AD7616_CONV_GPIO_Port, AD7616_CONV_Pin, GPIO_PIN_RESET);
    // 延时
    HAL_GPIO_WritePin(AD7616_CONV_GPIO_Port, AD7616_CONV_Pin, GPIO_PIN_SET);
}
```

此阶段 PA15 仅需配置为普通 `GPIO_Output`，目的是先验证并口读数、BUSY 检测、LCD 显示和 DDS 波形输出链路。

### 第二阶段：TIM2 PWM 硬件触发

为实现稳定的 500kSPS 固定采样率，将 PA15 切换至 TIM2_CH1 PWM 输出。TIM2 的 CH1 可通过 AF1 直接映射到 PA15，适合产生周期性 CONV 脉冲。

TIM2 配置（APB1 Timer Clock = 84MHz）：

```
PSC = 0, ARR = 167, Pulse = 83 (PWM mode 1)
实际采样率 = 84MHz / 168 = 500kHz
```

时序（±5V 量程）：

```
t=0:     计数器回绕 → CH1 输出高 → PA15 CONV↑ → AD7616 启动转换
t≈988ns: CCR1 匹配 → CH1 输出低 → PA15 CONV↓ → AD7616 开始转换
t≈1.7μs: BUSY↓（转换完成，t_conv ≈ 700ns）
t=2μs:   Update ISR 触发 → 读取 GPIOD->IDR
```

±5V 量程下 CONV↓ 到 ISR 之间有约 1μs 余量，时序安全。±10V 量程 t_conv 约为 1000ns，余量不足，不建议使用。

### ISR 内操作规范

HAL_TIM_PeriodElapsedCallback 中执行的操作为：
- 轮询 BUSY 引脚（超时计数 ≥20 则报错）
- 拉低 CS 和 RD
- 读取 GPIOD->IDR 获取 16 位数据
- 拉高 RD 和 CS
- 样本写入缓冲区，满 2048 点后置位 frame_ready

ISR 内不执行：
- FFT 运算（放置于主循环状态机）
- LCD 刷新
- 连续写入多个 DDS 寄存器

---

## DMA 说明

### 当前不使用 DMA 的原因

AD7616 通过 GPIO 并口（PD0~PD15）读取，STM32 的 DMA 控制器无法从 `GPIOD->IDR` 直接执行外设到内存的数据搬运——IDR 并非 DMA 可访问的标准外设数据寄存器。

CubeMX 中不应在 `Analog → ADC` 下配置片内 ADC + DMA，这与 AD7616 的数据路径无关。

### 当前采样策略

主循环进入 `APP_STATE_ACQUIRE` → 启动 TIM2 → ISR 逐点填充缓冲区 → 满 2048 点 → 停止 TIM2 → 切换到 `APP_STATE_ANALYZE` 执行 FFT。不依赖 DMA。

### 未来 DMA 迁移路径

仅当采集链路改为 DMA 可访问的外设（如 FSMC 并口或 DCMI）时，DMA 才有意义。推荐参数：
- DMA Mode: Normal（一帧采满即停，适合 FFT）
- Data Width: Half Word（16 位采样值）
- 不推荐 Circular 模式（FFT 分析期间缓冲区可能被新数据覆盖）

---

## LCD 接口说明

`lcd.c` 底层直接操作 GPIO 翻转 `SCL/SDA/CS/DC/RES/BLK`，未调用 `HAL_SPI_Transmit()` 等 SPI 外设接口，因此 CubeMX 中 LCD 六根引脚全部配置为 `GPIO_Output`，不开启 SPI 外设。

如需切换至硬件 SPI2（PB13=SCK, PB15=MOSI），除 CubeMX 配置外还需同步修改 `lcd.c` 的发送函数。

---

## CubeMX 配置验证

对照 `Core/Inc/main.h`（CubeMX 生成）与各驱动代码进行逐项审计：

### AD7616 — 22 个引脚，全部通过

驱动通过 `ad7616_port.h` 的 `#ifndef` 机制引用 CubeMX 生成的 `AD7616_Dx_GPIO_Port` 和 `AD7616_Dx_Pin` 宏，全部配对正确。

### AD9833 — 3 个引脚，全部通过

`ad9833.h` 使用 `#ifndef AD9833_FSYNC_PORT` 模式，CubeMX 生成的宏优先使用，全部配对正确。

### AD9834 — 6 个引脚，全部通过

全部引脚与 CubeMX Label 配对正确。

### LCD — 6 个引脚，全部通过

`lcd.h` 通过 `#if defined()` 链兼容 `LCD_SCL` 和 `SCL` 两套命名，当前使用 `LCD_*` 前缀。

### LED / KEY

| 用途 | CubeMX Label | STM32 引脚 | 状态 |
|---|---|---|---|
| 蓝灯 | LED_BULL | PB2 | 已配置 |
| 红灯 | LEC_RED | PC5 | 已配置 |
| 按键 | KEY | PA0 | 已配置（当前业务未使用） |

---

## 驱动头文件兜底机制

所有驱动头文件使用统一的 `#ifndef` 兜底模式：

```c
#ifndef AD9833_FSYNC_PORT
#define AD9833_FSYNC_PORT   AD9833_FSYNC_GPIO_Port
#define AD9833_FSYNC_PIN    AD9833_FSYNC_Pin
#endif
```

工作逻辑：
1. 若 CubeMX 已生成对应的 `_GPIO_Port` 和 `_Pin` 宏，`#ifndef` 不触发，直接使用 CubeMX 值
2. 若 CubeMX 未生成对应 Label，`#ifndef` 分支使用硬编码的 GPIO 值作为兜底，避免编译失败
3. 更换引脚时仅需修改 `.ioc` 并重新生成，驱动代码无需修改

---

## 实物接线说明

### AD7616
- DB0~DB15 按位序连接，不可跳位
- 围绕 P5 排针接线：PD0~PD15（数据总线）+ PC6~PC12（控制线）+ PA8~PA11（通道选择等）+ PA15（CONVST）
- BUSY 引脚必须连接到 MCU 输入，不可悬空
- RNG0/RNG1 和 CHS0~CHS2 使用跳线连接，避免焊死以便后续切换量程和通道
- SER/PAR 接地（选择并口模式）

### AD9833 / AD9834
- 两片 DDS 各自独立 SPI 总线，无共享信号线
- AD9833 P2 模块：FSYNC/SCK/SDATA 共 3 根，接 PA2/PA4/PA6
- AD9834 P4 模块：FSY/SCK/SDA/RST/FS/PS 共 6 根，FS/PS 已板载串联 100R 电阻
- 两片 DDS 均由板载晶振提供 MCLK，STM32 无需输出时钟
- AD9833 最高输出 12.5MHz（25MHz 主钟），AD9834 最高输出 37.5MHz（75MHz 主钟），均覆盖题目 20kHz~100kHz 范围

### LCD
- 六根信号线全部位于 P4 底部 PB10~PB15 区域
- 非硬件 SPI，为 GPIO 软件模拟时序
- CS/DC/RES 均需连接，不可固定电平（LCD 初始化依赖 RES 复位时序）
- BLK 连接至 MCU，可通过软件控制背光

---

## 调试注意事项

1. **TIM2 时钟频率**：APB1 总线时钟为 42MHz，但定时器时钟为 APB1 × 2 = 84MHz。若按 42MHz 计算 ARR 将导致实际采样率为目标值的一半。

2. **CONV 脉冲稳定性**：主循环中软件翻转 CONV 的采样间隔受中断和代码分支影响波动较大，改用 TIM2 PWM 硬件触发后方可达到稳定的 500kSPS。硬件未就绪时可先用阻塞式采样验证链路完整性。

3. **AD9834 RST 初始电平**：AD9834 的 RST 为低有效复位，MX_GPIO_Init 中必须将其初始输出设为高电平（`GPIO_PIN_SET`），否则芯片持续处于复位状态无输出。

4. **LCD Label 兼容**：`lcd.h` 的 `#if defined()` 链支持多套命名自动适配，切换 CubeMX Label 命名风格时无需修改驱动。

5. **FFT 去直流偏置**：AD7616 采集数据含有直流偏置分量，若不做去均值处理直接进入 FFT，0Hz bin 幅值会明显偏高并影响频谱显示。`dsp_process.c` 中先减去采样均值再施加 Hann 窗可有效去除直流分量。

6. **AD7616 控制线空闲电平**：CS/RD/WR/RST 均为低有效信号，MX_GPIO_Init 中必须初始化输出为高电平，否则上电后 AD7616 可能被意外选中或复位，导致总线状态异常。
