# 当前框架架构与限制

原架构事实基线：`d29419be990aa562bcbacabb17ac9c78b31a0908`；文档库提交为
`bb9c659fff03de48d35682f949bbed8b31e5473c`。下述 CAN 条目描述 2026-10-12
当前未提交工作树，证据和未验证边界见对应完成记录。
来源：来源清单中的 framework `README.md`、`bus/dev/*.h`、`port_bsp/*`、
`board_support/demo/*`、`component/ring_buf.*`。源码存在不等于构建或硬件验证通过。

## 所有权

| 子模块内容 | 当前职责 | 宿主承担的内容 |
| --- | --- | --- |
| `bus/dev` | GPIO/UART/SPI/I2C/CAN/TIM/PWM 封装、ops 和静态设备表 | 实例与物理资源绑定；部分设备查询入口 |
| `port_bsp` | STM32 HAL 适配、资源/instance 类型、MSP 和 DMA helper | HAL SDK、板级头文件、资源查询、向量接入 |
| `common/usr_check.h` | return guard 宏 | `usr_common.h` 公共类型、日志依赖 |
| `component/ring_buf` | 软件生产与 circular DMA half-buffer commit | 存储分配、DMA 与前台调度/同步条件 |
| `log/my_log.h` | logger 声明及宏 | logger 实现、时钟与输出后端 |
| `board_support/demo` | 无真实板级资源的桩 | 真实板级支持和系统服务 |

调用关系是宿主装配抽象设备、bus wrapper 经 `ops(ctx, ...)` 调用 adapter，adapter 使用
宿主资源与 HAL；同时 adapter 会调用宿主 `usr_port_board_*_find_instance()`。
因此不能用单向目录图声称子模块已经彻底消除宿主依赖。

## 当前接口事实

- UART/SPI 初始化接收可选配置指针，ops 的参数仍是 `const void *cfg`；完整类型在公开头文件快照中。
- UART 有 NONE、DMA_CIRCULAR、INTERRUPT 模式，以及配置和收发 control 命令。
- SPI 保留事务片选和多帧接口；参数长度是字节，HAL 位宽与帧数转换由 adapter 实现。
- I2C 公开接口使用 7 位地址及 1/2 字节寄存器宽度；源码中的旧重试实现有注释块，不能根据旧指南宣称自动恢复重试已生效。
- CAN 公开接口使用完整帧，支持 Classic CAN/CAN FD、标准/扩展 ID、数据/远程帧、BRS、
  可选初始化配置与状态查询；`send` 成功仅表示帧已提交硬件 TX FIFO。
- CAN adapter 将 FDCAN mode、Message RAM 数量/元素大小、TX FIFO/Queue mode、filter
  destination 和 RX location 全部放在 board resource；轮询与 ISR 均按同一 RX location
  处理 FIFO0、FIFO1 或 dedicated RX buffer。
- 中断接收使用静态完整帧 SPSC 队列；队列满时丢弃新帧并累计 overflow，ISR 不执行日志或协议解析。
- TIM 与 PWM 现在有独立 `port_hal_tim` 和 `port_hal_pwm` 文件、resource 与 instance 类型。
- PWM 的 `DEV_PWM_CHANNEL_1..4` 当前数值是 `0..3`，旧记录的 `1..4` 已不适用；port 校验和 HAL 映射仍需结合当前实现阅读。
- TIM/PWM 资源使用静态 `timer_clock_hz`、PSC、ARR；PWM 更新原始 compare 计数。旧频率/占空比换算、输入捕获和 owner/claim 方案不属于当前公开能力。

## 已知移植差距

- UART/SPI/CAN/TIM/PWM 设备数量由公共头文件的固定 enum/table 确定，并非任意实例注册。
- TIM adapter 仍包含 `TIM2_IRQHandler`、TIM2 callback 分支和遗留 `htim2` 声明；PWM 头文件仍声明 `htim5`。
- `SYSTEM_TIMER` 仅在当前 TIM MSP init 中跳过宿主系统定时器；不能据此宣称全部 init/deinit/callback 路径都适配了 RTOS。
- demo 的 timer getter 形状与当前 bus timer getter 不一致，且仅提供 NULL/no-op 桩；没有独立构建定义或本次编译证据。
- 当前仓库没有真实板级支持目录；H743 和 KG200Z 支持包已经移到宿主/历史，README 的旧示例不构成当前支持列表。
- ring buffer 使用 ARM DMB 或编译器 barrier、position/epoch、drop-oldest 和 DMA 半缓冲安全窗口。这不等于任意多线程、cache MCU 或 RTOS 下线程安全。
- CAN 公共层主机测试与 H743 固件构建不能证明真实收发、ACK、过滤器或 bus-off 恢复；这些仍需目标板和 CAN 分析仪观察。

这些差距作为真实限制记录；除 CAN/FDCAN 新增范围外，本次没有改写固件来消除它们。
