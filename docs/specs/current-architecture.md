# 当前框架架构与限制

原架构事实基线：`d29419be990aa562bcbacabb17ac9c78b31a0908`；文档库提交为
`bb9c659fff03de48d35682f949bbed8b31e5473c`。下述 CAN 条目描述 2026-10-12
当前未提交工作树，证据和未验证边界见对应完成记录。
SPI DMA 增补为 fresh 工作树观察：framework HEAD
`9bb4b31fd57696a34b0126505ef2ff4a27cffe0a`、H743 host HEAD
`1f1b738d30b936f015eb689568ad57b97bda072d`，均为 dirty；见
[SPI DMA 完成记录](../completions/2026-10-12-spi-dma.md)。
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
- SPI 保留同步事务片选和多帧接口；参数长度是字节，HAL 位宽与帧数转换由 adapter 实现。
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

## SPI 裸机多模式当前工作树契约

本节为 2026-10-13 fresh 观察，来自 `bus/dev/dev_spi.[ch]`、`dri_ops.h`、
`port_bsp/port_hal_spi.[ch]` 和 `port_hal_dma.[ch]`。2026-10-12 的 RTOS 同步实现保留在
历史完成记录中，不再描述当前代码。

- `spi_device_config_t` 为 TX/RX 分别提供 `POLLING`、`INTERRUPT`、`DMA` 三种固定模式；
  修改模式需要 deinit 后重新 init。H743 的 SPI2/SPI3/SPI4 默认仍为 DMA+DMA。
- resource 的 `rx_dma/tx_dma` 独立可选。只有对应方向配置为 DMA 时才要求、初始化、
  link、启用 IRQ、abort 和 deinit 该 descriptor；仅 RX DMA、仅 TX DMA、无 DMA 均合法。
- `dev_spi_ops_t` 的 claim/release/is_owner 仍由 adapter 提供，但 SPI 路径不再依赖
  FreeRTOS mutex、semaphore、task owner、scheduler 状态或 task notification。裸机
  `claimed` 只防止同一前台调用链递归进入，不提供多任务互斥。
- instance 保留 `IDLE/ACTIVE/COMPLETED/ERROR/ABORTING/FAULTED`、result、计数和方向 DMA
  readiness。claim 与前台/ISR 状态交换只使用短 PRIMASK 临界区，并以 DMB 发布终态。

单向 TX/RX 分别调用配置模式对应的 polling、IT 或 DMA HAL API。不等长 TX/RX 先完整
执行 TX，再执行 RX，两个阶段共享一个由 `HAL_GetTick()` 无符号差值计算的总超时。
等长全双工仅 DMA+DMA 调用 `HAL_SPI_TransmitReceive_DMA()`，IT+IT 调用
`HAL_SPI_TransmitReceive_IT()`；其余七种混合组合统一回退到阻塞
`HAL_SPI_TransmitReceive()`，不启动 DMA，也不检查 DMA buffer。RX DMA 与阻塞 TX 只在
先 TX 后 RX 的两阶段事务中组合；同一个全双工阶段不实现 CPU 写 TXDR 的 H7 专用引擎。

所有公开事务仍同步返回，且只允许前台在 `IPSR/PRIMASK/BASEPRI/FAULTMASK` 均为 0 的
上下文调用。`timeout_ms=0` 立即返回 timeout，不启动传输。8 bit 长度按字节转换为 HAL
frame；16 bit 要求偶数字节长度和 2 字节地址对齐。只有实际 DMA 方向调用宿主
`dma_buffer_valid`；adapter 先检查 8/16 bit 长度与地址对齐，H743 policy 再拒绝 DTCM、
D-Cache 开启状态和越界地址。

IT/DMA phase 在 ACTIVE 状态等待 HAL SPI completion/error callback；DMA TC 不是成功
终点，SPI EOT callback 才发布成功。超时或错误先切换 ABORTING，再调用阻塞
`HAL_SPI_Abort()`。恢复预算从首次错误或超时起累计；HAL abort 返回后若总耗时已达到
10 ms 则进入 FAULTED，后续 DMA ABORT 收敛轮询也受同一预算约束。阻塞 HAL abort 本身
不可中断，因此 10 ms 不是函数返回的墙钟硬截止。返回 IDLE 前必须确认 SPI READY、已用
DMA stream EN=0、DMA handle READY/RESET，并清 SPI/DMA flags 与已启用的 NVIC pending。
恢复失败进入 FAULTED，后续事务返回 `USR_ERR_STATE`，只有 deinit/reinit 可以恢复；
ABORTING 下的普通完成 callback 与任何迟到、重复 callback 都不能发布成功。

这是 SPI adapter 裸机化，不是从整个宿主工程删除 FreeRTOS。若应用仍运行 RTOS，必须
由一个固定任务串行访问同一 SPI，或在 adapter 外提供应用级互斥。物理 pin、stream 与
硬件验收留在宿主工程；当前证据见
[SPI 裸机多模式完成记录](../completions/2026-10-13-spi-bare-metal-modes.md)。

## 已知移植差距

- UART/SPI/CAN/TIM/PWM 设备数量由公共头文件的固定 enum/table 确定，并非任意实例注册。
- TIM adapter 仍包含 `TIM2_IRQHandler`、TIM2 callback 分支和遗留 `htim2` 声明；PWM 头文件仍声明 `htim5`。
- `SYSTEM_TIMER` 仅在当前 TIM MSP init 中跳过宿主系统定时器；不能据此宣称全部 init/deinit/callback 路径都适配了 RTOS。
- demo 的 timer getter 形状与当前 bus timer getter 不一致，且仅提供 NULL/no-op 桩；没有独立构建定义或本次编译证据。
- 当前仓库没有真实板级支持目录；H743 和 KG200Z 支持包已经移到宿主/历史，README 的旧示例不构成当前支持列表。
- ring buffer 使用 ARM DMB 或编译器 barrier、position/epoch、drop-oldest 和 DMA 半缓冲安全窗口。这不等于任意多线程、cache MCU 或 RTOS 下线程安全。
- CAN 公共层主机测试与 H743 固件构建不能证明真实收发、ACK、过滤器或 bus-off 恢复；这些仍需目标板和 CAN 分析仪观察。

这些差距作为真实限制记录；本次文档侧任务没有改写固件来消除它们。
