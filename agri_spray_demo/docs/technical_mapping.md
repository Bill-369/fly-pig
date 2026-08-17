# fly-pig 与植保喷洒演示的技术映射（第一阶段）

> 状态：仓库静态审查完成；尚未新增固件、原理图或 Proteus 工程。本文把“已存在”“拟新增”“未来扩展”严格分开。

## 1. 审查范围与证据边界

本阶段阅读了根目录 `README.md`、`USER/`、`BSP/include`、`BSP/source`、`PID/`、`AHRS.c/.h`、`Attitude_solving/`、STM32 CMSIS/标准外设库的系统时钟文件，以及 `fly-pig.uvprojx/.uvoptx` 和已有 Keil 构建日志。仓库内没有 ADC 驱动、CubeMX `.ioc`、原理图/PCB 源文件或可验证的 Proteus 工程。

两个指定参考仓库的远程读取也已尝试，但当前容器访问 GitHub 时返回 `CONNECT tunnel failed, response 403`。因此本阶段**没有声称已核验** Avem 或 DroneController 的具体实现、许可证或电路细节，也没有复制其代码；待网络可用时只审查其 IMU、PWM/ESC、ADC/电源监测和 schematic/PCB 目录，并在归属文件中记录具体路径与许可证。

## 2. 当前 fly-pig 已经有什么

### 2.1 硬件与工具链事实

- Keil 目标是 `STM32F401RETx`（Cortex-M4F，512 KiB Flash、项目配置给出 96 KiB SRAM），宏为 `STM32F401xx`，使用 STM32F4 标准外设库，而不是 HAL。
- CMSIS 配置为 8 MHz HSE、PLL M=8/N=336/P=4，目标 `SystemCoreClock=84 MHz`。TIM3 配置也以 84 MHz 定时器时钟、PSC=83 为 1 MHz 计数基准。
- 独立 Keil5 工程 `fly-pig.uvprojx` 已存在；仓库保存的历史构建日志显示 Arm Compiler 5.06、Keil 5.36 曾得到 0 error/0 warning，输出名为 `sizhou`。这只是 2022 年的历史证据，不等于本环境重新构建成功。
- README 所列实物链路为 GY-86、SSD1305 OLED、ESP8266-01S、T8FB/R8FM、四个 A2212/13T 电机及 20 A ESC。

### 2.2 软件架构

`USER/main.c` 先初始化 BSP，再启动 uC/OS-II，并建立三个任务：

1. `GY86_Thread`：采样 GY-86，调用 Madgwick 姿态更新；
2. `ChangeMotor_Thread`：更新 PID 参数，执行串级俯仰 PID、动力混控和四路输出；
3. `ANO_Thread`：组装匿名上位机格式的姿态、IMU、磁力计、PID/PWM遥测。

已有模块及真实程度：

| 能力 | 已存在的实现 | 当前审查结论 |
|---|---|---|
| IMU/AHRS | MPU6050 + HMC5883，经 I2C3；四元数/Madgwick 数据融合 | 有源码；未在本环境接硬件验证 |
| PID | 角度/角速度相关数据结构、俯仰串级 PID、四电机混控 | 有源码；当前主要看到 pitch 通道，不能宣称完整三轴闭环已验证 |
| 遥控 | TIM1_CH4 输入捕获 PPM，缓存 8 通道 | 有实现，但 `PPM_Init()` 在 `main` 中被注释，初始化与中断路径仍需复核 |
| 电调 | TIM3 四通道 50 Hz PWM，比较值驱动 PA6/PA7/PB0/PB1 | 有源码；适合复用“定时器 PWM”方法，不把 ESC 脉宽直接当泵调速 PWM |
| 显示 | SSD1305 OLED，经 I2C1 | 有驱动；Proteus 器件兼容性尚未验证 |
| 通信 | USART1 用于高度/测距输入；USART6 + DMA/中断用于 ESP8266、PID/遥测 | 存在多用途/中断处理函数重叠风险，独立演示需只保留一个明确 debug 通道 |
| 高度 | `bsp_ray` 有串口/I2C测距命令和帧接收；`USER/altitude.c` 基本为空 | 只能说有接口探索，不能说高度控制已完成 |
| RTOS/调试 | uC/OS-II、SEGGER SystemView/RTT | 已纳入工程；演示版可用周期任务或简单调度，状态机不依赖 RTOS |
| ADC/电源 | 无 | 必须新增 |
| 喷洒状态机/联锁 | 无 | 必须新增 |

### 2.3 已确认的引脚/外设占用

| 功能 | 引脚 | 外设 | 备注 |
|---|---|---|---|
| OLED | PB6/PB7 | I2C1 SCL/SDA | 400 kHz |
| GY-86 | PA8/PC9 | I2C3 SCL/SDA | MPU6050 + HMC5883 |
| PPM | PA11 | TIM1_CH4 | 源码实际配置；头文件注释称 PA9/TIM1_CH2，注释已过时 |
| ESC 1..4 | PA6, PA7, PB0, PB1 | TIM3_CH1..4 | 50 Hz，1 µs 计数 |
| USART1 | PA9/PA10 | USART1 TX/RX | 高度/测距实验；与头文件中过时 PPM 注释冲突，但不与实际 PA11 冲突 |
| USART6 / ESP8266 | PC6/PC7 | USART6 TX/RX | DMA TX 已配置 |
| LED | PA5 | GPIO | 已有板载状态灯接口 |
| 校准按键 | PC13 | GPIO | GY-86 校准路径 |
| 软件计时 | TIM2/TIM5 | Timer | 分别为 AHRS/PID 时间基准；TIM4 为阻塞延时 |

**关键结论：**项目没有对 ADC 资源做初始化，因此 ADC1 的 PC0/PC1/PC2（通道 10/11/12）在当前源码扫描中未占用，可作为 F401 独立演示的三个模拟输入候选。PB8/TIM10_CH1 可作为泵 PWM 候选；PC4 可作蜂鸣器候选；PA5 可复用状态 LED；PC6/PC7 可复用 USART6 debug。候选必须在第二阶段结合 STM32F401RE 数据手册、封装和最终显示接口再做一次 AF/电气冲突核对，届时才形成正式 `pin_definition.md`。

## 3. 与植保演示的映射

### 3.1 可直接复用（思想或隔离后的驱动）

- **飞控上下文：**保留 GY-86 → AHRS → PID → 四路 ESC 的原系统作为无人机稳定飞行背景，不把它移植进 Proteus 喷洒闭环。
- **PWM方法：**复用标准外设库的 RCC/GPIO AF/TIM PWM 初始化方式；泵使用独立定时器通道，避免改动 TIM3 四路电调。
- **通信方法：**复用 USART 的初始化与发送思路；Demo 输出可读文本，而不强制使用匿名上位机二进制协议。
- **显示方法：**F401 实物路线可复用 I2C/OLED 抽象；Proteus 路线优先选择其器件库中可实际运行的字符 LCD。
- **任务分层：**沿用“采集—计算—执行/遥测”的周期结构，但用明确状态机集中管理安全联锁。

“复用”不表示直接把原文件链接进新工程；演示工程将保持独立，必要的接口会以小型适配层实现并标注来源。

### 3.2 本次必须新增

1. 三路 ADC 采样与定标：电池百分比、药液百分比、模拟作业高度；
2. `INIT → SELF_CHECK → READY → SPRAYING → WARNING/SAFE_STOP` 状态机；
3. 低电池、低药液、高度越界的集中告警码、阈值和喷洒硬联锁；
4. 泵 PWM 输出（经 MOSFET/驱动级，MCU 不直接驱动电机）、蜂鸣器和 LED；
5. LCD/OLED 状态页与 USART 文本遥测；
6. 故障恢复策略：建议安全停机锁存，输入恢复后人工确认，防止阈值附近反复启停；
7. 可复现连接表、F401↔F103 功能映射、仿真用例及答辩材料。

### 3.3 仅作为未来扩展（未实现）

- AHRS/PID 对姿态或高度的喷量补偿；
- GPS 航线、流量计闭环、压力传感器、药泵电流诊断；
- ESP8266 云端作业记录或遥控喷洒；
- 四电机与整机飞行动力学联合仿真；
- AI、视觉识别、变量施药算法。

这些能力不得在答辩中表述为已完成。

## 4. 可验证性分层

| 层级 | 本阶段/本环境可验证 | 不能据此声称 |
|---|---|---|
| 静态事实 | 工程目标、源文件、宏、引脚、定时器参数、历史 build log | 当前 Keil 编译通过、硬件可飞 |
| 后续主机测试 | 纯 C 状态机、阈值边界、锁存/恢复、ADC 数值换算 | MCU 外设时序正确 |
| 后续 Proteus | 若本地有相应模型：ADC 电位器、PWM波形、LCD、UART、报警场景 | 真实传感精度、泵流量、飞行动力学 |
| 后续 Keil | 用户本地 Keil5 构建 HEX | 云端已执行 Keil（当前没有 Keil） |
| 实物 | 需要 F401 板、分压/驱动级、传感器和泵测试 | 当前仓库或仿真等同硬件实测 |
