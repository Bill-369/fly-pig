# 植保喷洒 Demo 设计决策与实施计划（第一阶段）

> 本文件是决策记录，不是“已经实现”的说明。第一阶段只创建本文件和 `technical_mapping.md`。

## DD-001：与原飞控隔离

**决定：**所有新增内容放在根目录 `agri_spray_demo/`；不修改原 `USER/`、`BSP/`、PID、AHRS 或 Keil 工程。

**理由：**喷洒演示验证的是“状态采集 → 判断 → 执行 → 保护 → 显示/通信”，不是重写飞控。隔离可防止新增 ADC、泵 PWM、显示资源破坏现有姿态与四电机时序，也便于评委区分已有飞控与本次新增。

## DD-002：F401 是产品/集成主线，F103 是 Proteus 功能验证首选

**决定：**

- 文档、真实集成接口和最终目标 MCU 均保持 **STM32F401RETx / 84 MHz**；
- Proteus 首选建立清楚标注的 **STM32F103C8T6 功能等价仿真版本**，前提是用户本地 Proteus 器件库确认该模型、ADC、定时器、LCD 和 UART 可运行；
- 若用户的 Proteus 版本经最小冒烟工程确认 F401 模型可靠，则可追加 F401 仿真目标，但不因此删除 F103 路线。

**为什么不是现在直接宣称 F401 可仿真：**当前 Linux 容器未安装 Proteus，也无法查看用户的器件库或验证 `.pdsprj`。器件支持随 Proteus 版本/库而异。F103 通常更适合作为教学仿真的低风险路线，但这仍需在目标机器验证，不能伪装成已验证结论。

**语义边界：**F103 仿真只证明状态机和 I/O 交互，不证明 F401 二进制、时钟、引脚或硬件已验证。两套固件共享与 MCU 无关的 `App` 状态机/换算逻辑，各自使用独立 HAL/BSP。

### 初步功能映射（第二阶段需按数据手册定稿）

| 功能 | F401 候选 | F103 仿真候选 | 映射原则 |
|---|---|---|---|
| Battery ADC | PC0 / ADC1_IN10 | PA0 / ADC1_IN0 | 同为 12-bit ADC 输入 |
| Liquid ADC | PC1 / ADC1_IN11 | PA1 / ADC1_IN1 | 电位器 0–3.3 V |
| Altitude ADC | PC2 / ADC1_IN12 | PA2 / ADC1_IN2 | 只做线性模拟量 |
| Pump PWM | PB8 / TIM10_CH1 | PA6 / TIM3_CH1 | F103 Demo 无四路 ESC，F401 避开现有 TIM3 |
| Debug UART | PC6/PC7 / USART6 | PA9/PA10 / USART1 | Virtual Terminal 只需 TX，RX留作命令/确认 |
| Status LED | PA5 GPIO | PC13 或 PB12 GPIO | 依板/模型选择有效电平 |
| Buzzer | PC4 GPIO | PB0 GPIO | 经晶体管驱动实物负载 |
| Display | 先保留 I2C1 OLED；仿真可改字符 LCD | 16×2 LCD 4-bit GPIO | 以本地 Proteus 模型可用性为准 |

注意：F103 的 PA6 是“仅仿真版泵 PWM”，不对应 F401 原工程 PA6 的 ESC1。映射表表达的是**功能等价**而不是板级同引脚。

## DD-003：状态机和安全策略

**决定：**采用单一、可测试的状态机，而不是散落的 `if/else`：

- `SYSTEM_INIT`：初始化输出，泵强制关；
- `SYSTEM_SELF_CHECK`：验证 ADC 是否在合理电气范围，建立滤波初值；
- `SYSTEM_READY`：输入安全，等待人工喷洒命令；
- `SYSTEM_SPRAYING`：按设定占空比输出 PWM；
- `SYSTEM_WARNING`：显示/串口报告非锁存提示（若最终需求确有此类提示）；
- `SYSTEM_SAFE_STOP`：任一关键联锁触发，PWM 立即为 0，告警并锁存。

低电池、低药液、作业高度越界均为关键联锁。建议恢复采用“输入回到带回差的安全范围 + 人工 ACK”而非自动重启：这使现场行为确定，并避免液位波动或 ADC 噪声导致泵反复启停。阈值、回差、连续样本确认次数将在实现时集中配置；在真实传感器/电池规格确定前不伪造工程精度。

## DD-004：ADC 输入只代表演示量

- Proteus 使用 0–3.3 V 电位器输入；纯逻辑测试直接注入工程量。
- F401 实物电池输入必须有分压、RC 滤波和过压保护，且分压后不得超过 VDDA；药液/高度传感器同样需要明确输出范围。
- 百分比可先用分段或线性演示换算并饱和到 0–100%；高度只表示“模拟高度信号”，不宣称真实测距精度。

## DD-005：泵接口不是直接驱动

MCU PWM 只驱动逻辑级 MOSFET/栅极驱动器；泵使用独立电源、共地、续流/浪涌抑制，默认下拉确保复位关断。Proteus 可用 DC Motor、LED 和示波器/Logic Analyzer 三者之一或组合表现 PWM；评价重点是占空比和故障时归零，而不是仿真水力学。

## DD-006：显示与通信选择

- 仿真显示优先 HD44780 兼容 16×2/20×4 字符 LCD，因为答辩信息少且目标是可复现；只有确认本地 SSD1305 模型可用才用 OLED。
- UART 周期输出 `battery, liquid, altitude, spray, alarm, state` 的稳定文本行，供 Virtual Terminal 留证。
- F401 集成路线可继续用现有 OLED/I2C1 和 USART6，但必须解决现有 USART6 多个用途/IRQ实现的所有权，不能简单拼接进原主工程。

## DD-007：参考项目使用原则

指定参考：

- `https://github.com/but0n/Avem`
- `https://github.com/FPV-Drone-STM32F411/DroneController`

当前网络 403，尚未完成内容与许可证核验。恢复访问后仅提炼架构/硬件设计模式（IMU、电源检测、ADC、PWM/ESC、模块边界和 schematic/PCB 表达）；不大规模复制源码或整板设计。任何实际借鉴都在 `docs/open_source_attribution.md` 精确列出项目、URL、许可证、文件/章节、借鉴点与本项目新增内容。许可证未核验前不写猜测性结论。

## 2. 第二阶段实施计划与最终文件清单

按“先可测试逻辑，再 MCU 外设，再仿真材料”的顺序：

1. **基线与规格**：补查 F401/F103 数据手册 AF、ADC 和电气限制；联网核验两个参考项目及许可证；冻结阈值/回差/故障优先级。
2. **纯 C App**：实现传感器结构、告警位/优先级、状态机、换算与人工确认接口；用主机单元测试覆盖正常、三类故障、边界和恢复。
3. **F103 Proteus BSP**：ADC×3、TIM PWM、GPIO报警、USART、字符 LCD；建立独立 Keil target 和 HEX 生成说明。
4. **F401 集成 BSP**：保持 STM32F401RE，使用不冲突候选资源；只在独立 Demo 工程中实现，不改原飞控。
5. **仿真复现**：先检查用户 Proteus 版本的器件模型；无法可靠生成/打开工程时只给详尽连接表，绝不伪造 `.pdsprj`。
6. **验证与答辩**：记录至少五个场景的输入、状态、PWM、LCD、UART和告警预期；区分主机测试、Keil build、Proteus仿真和实物测试。

计划文件树（后续逐步创建，不代表本阶段已有）：

```text
agri_spray_demo/
├─ README.md
├─ firmware/
│  ├─ App/                 # MCU无关状态机、换算、配置
│  ├─ Core/                # 调度与公共接口
│  ├─ Drivers/F401/        # 产品/集成目标 BSP
│  ├─ Drivers/F103/        # Proteus功能等价 BSP
│  ├─ Tests/               # 主机逻辑测试
│  └─ Keil/                # 两目标工程或明确导入说明
├─ proteus/
│  ├─ README.md
│  ├─ connection_table.md
│  └─ simulation_cases.md
├─ schematic/
│  ├─ system_architecture.md
│  ├─ hardware_design.md
│  ├─ pin_definition.md
│  └─ bom.md
├─ docs/
│  ├─ technical_mapping.md
│  ├─ design_decisions.md
│  ├─ verification.md
│  └─ open_source_attribution.md
└─ presentation_assets/
   ├─ README.md
   ├─ screenshot_checklist.md
   └─ defense_script.md
```

## 3. 验证承诺与限制

### 后续可在当前容器真实验证

- 目录隔离、Git diff、文档链接；
- App 层由宿主 C 编译器编译并执行的状态机/边界测试；
- 若提供交叉编译工具，可做对应目标的编译/链接和静态检查；
- 引脚表与代码常量的一致性脚本检查。

### 当前容器不能宣称完成

- **Keil：**未安装 µVision/Arm Compiler 5；后续必须标注 `NOT VERIFIED IN KEIL`，除非在可用 Keil 环境实际重建并保留日志。
- **Proteus：**未安装 Proteus，无法确认器件模型或打开/运行 `.pdsprj`；不会生成假工程。
- **硬件：**没有 F401 板、传感器、ESC或泵，不能声称电气、流量、飞行或抗干扰实测。
- **参考仓库审查：**本阶段因 GitHub 403 未完成；网络恢复前不据此做具体归属断言。

## 4. 第一阶段退出条件

已满足：扫描原仓库、梳理 MCU/时钟/架构/引脚、识别 ADC 缺口、给出 F401/F103 路线和后续计划；仅新增两份分析文档。此处停止，不提前生成固件、HEX、Proteus 工程或大量占位文件。

## Phase 2 implementation update

The implemented phase-2 state machine is documented in `firmware/common/README.md`. It uses `INIT`, `READY`, `SPRAYING` and latched `SAFE_STOP`; `WARNING` is reserved but unused because all four present demo faults are safety interlocks. The earlier conceptual `SELF_CHECK` is represented by validation of the first sample while in `INIT`, rather than a separate public enum value. Final pin audit results are in `schematic/pin_definition.md` and supersede the preliminary table above.
