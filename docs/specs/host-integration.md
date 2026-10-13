# 宿主工程接入契约

基线与来源：framework `usr_port.h`、`usr_port_hal.h`、`board_support/demo/*`、
`port_bsp/*.h`、`port_hal_tim.c`、`log/my_log.h`，详见来源清单。

SPI DMA 增补来自 2026-10-12 fresh 工作树：framework HEAD
`9bb4b31fd57696a34b0126505ef2ff4a27cffe0a`、H743 host HEAD
`1f1b738d30b936f015eb689568ad57b97bda072d`，均为 dirty。S039/S040/S045/S052/S053/S060/S061
保留固定基线 provenance；当前观察及验证归属见
[SPI DMA 完成记录](../completions/2026-10-12-spi-dma.md)。

## AI 入口

在宿主工程的 AI 工作入口加入相对路径指引，例如：

```text
修改 dev_fram 框架前，读取 Usr/hal/AGENTS.md、Usr/hal/docs/README.md，
并按其索引核对当前架构、接口、来源版本和历史验证边界。
```

实际目录由宿主决定。子模块内部链接始终相对于子模块根目录，不绑定 KG200Z 路径或某台电脑。

## 编译与板级输入

宿主必须提供与实际 MCU 相符的 HAL/SDK、include 搜索路径及源文件列表；不能把
demo 桩与真实 BSP 同时作为同名强实现链接。根据所需模块选择源文件，不能将旧
README 中“修改 CMake 路径即可”解释为无需核对契约。

- `usr_common.h` 提供 `usr_status_t`、`USR_*` 值、逻辑 pin、callback、time 类型及所需标准类型。
- `board_support.h` 提供 HAL 类型可见性、板级声明和 adapter 所需查询契约。公共 bus 头通过 `dri_ops.h` 依赖 `usr_common.h`，宿主不能借此向公共层引入厂商类型。
- BSP 定义实际 resource 表、instance 存储、设备 `ctx/ops` 绑定，以及 UART/SPI/I2C/CAN/TIM/PWM 的 instance/DMA 查询。
- GPIO/I2C/time 查询与 platform init/system reset 等入口按实际公开声明由宿主实现；不要照抄 demo 中不匹配的 timer 声明。
- 使用 logger 时提供 `zlog`、level、tick、输出等实现；只存在 `my_log.h` 不能证明日志已可用。

## IRQ、MSP、时钟与 DMA

宿主为每个启用外设明确一个 IRQ/vector/MSP 所有者，核查哪些 HAL hooks 已由 adapter 强定义。
UART 物理 IRQ 经当前 `port_hal_uart_irq(handle)` 接入；DMA callbacks 和错误 callbacks
按返回 bool 的 ownership 协议分发。不得同时分发 port 和 legacy owner。

CAN/FDCAN 宿主还必须提供唯一 HAL handle、MSP 委派、IRQ vector 和 HAL callback owner。
CAN resource 负责 mode、GPIO/clock/IRQ、完整 Message RAM 布局、filter destination、RX location
和静态队列；adapter 在初始化前拒绝不一致的 route、元素容量和 Message RAM 预算。
若工程已弃用 `.ioc`，它不能作为资源事实源或验证输入；实际 C resource、启动调用链和链接符号才是当前配置。

当前 TIM 仍强定义 TIM2 vector 和 period callback，因此换板或使用 HAL TIM timebase 前须检查
重复符号、资源查询、未绑定 handle 和 `SYSTEM_TIMER` 的覆盖范围。这个问题是接入限制，
本次文档整理未解决它。

宿主必须确认 DMA channel/request、storage 寿命、缓存/对齐、IRQ 延迟和 ring 消费时序。
DMA 半缓冲 commit 要求偶数容量且提交长度为容量一半；完整接收行为需目标板观察。

## SPI 裸机多模式接入

`spi_device_config_t` 新增固定的 `tx_mode/rx_mode`，取值为 polling、interrupt 或 DMA；
所有指定 initializer 都应显式填写，修改模式前先 deinit。当前 H743 index 0/1/2 分别
绑定 SPI4/SPI2/SPI3，默认均为 DMA+DMA。custom ops 和调用者需要随新结构重新编译。

`usr_port_hal_spi_resource_t` 的 `pins[3]` 顺序仍为 SCK/MISO/MOSI。RX/TX DMA descriptor
改为分方向资源：只有对应 mode 为 DMA 才必须有效，允许只提供 RX、只提供 TX 或完全不
提供 DMA。每个启用的 descriptor 仍须使用独立 handle/stream/IRQ、`DMA_NORMAL` 和正确
direction；adapter 会复制 descriptor，并按 8/16 bit 设置 byte/halfword alignment。
default config 本身必须与 resource 匹配，即使 init 时传入另一份 runtime config。

删除 `usr_spi_sync.[ch]`、board sync bind、`sync_ops/sync_ctx` 和 FreeRTOS 测试替身。
宿主只需提供 `dma_buffer_valid(buffer, length, receive)` policy。adapter 内部的裸机
claimed 非递归且不提供任务互斥；运行 FreeRTOS 的宿主必须保证只有一个固定任务访问
同一 SPI，或在调用层自行串行化。

所有事务只允许前台、全局中断可用且 `IPSR/PRIMASK/BASEPRI/FAULTMASK` 均为 0 的上下文。
IT/DMA 对外仍同步等待，依赖 SysTick、SPI IRQ 和实际启用的 DMA IRQ。SPI IRQ 在任一
方向使用 IT 或 DMA 时启用，因为 DMA TC 后仍由 SPI EOT 完成；只为 mode=DMA 的 stream
配置 DMA IRQ。`timeout_ms=0` 不启动 HAL 传输。

执行矩阵如下：

| 调用形态 | HAL 路径 |
| --- | --- |
| 仅 TX / 仅 RX | 按对应方向 mode 选择 polling、IT 或 DMA |
| TX/RX 非等长 | 先 TX 后 RX，各用自己的 mode，共享总 timeout |
| 等长 DMA+DMA | `HAL_SPI_TransmitReceive_DMA` |
| 等长 IT+IT | `HAL_SPI_TransmitReceive_IT` |
| 等长其他七种组合 | 阻塞 `HAL_SPI_TransmitReceive` |

混合全双工回退不检查 DMA buffer，也不启动 DMA。TX polling 后 RX DMA 可用于命令后读；
不要把它解释为同一阶段的 RX DMA + CPU TX。master RX-only HAL 能产生 SCK，但 MOSI
dummy 值不作保证；协议要求指定 dummy 字节时使用显式等长 TxRx。

当前 H743 DMA policy 接受完整落在 AXI SRAM `0x24000000..0x24080000`、D2 SRAM
`0x30000000..0x30048000`、D3 SRAM `0x38000000..0x38010000` 的 buffer；TX 另允许 Flash
`0x08000000..0x08200000`。RX 不允许 Flash，DTCM、跨界、零长度、NULL 和 D-Cache 开启
状态均拒绝。该 policy 只检查实际执行 DMA 的 buffer；它不提供 cache maintenance。

唯一 SPI vector 经 `usr_spi_irq_dispatch()` -> `usr_port_hal_spi_irq()` ->
`HAL_SPI_IRQHandler()`，DMA vector 经 `usr_port_hal_spi_dma_irq()` ->
`port_hal_dma_irq()`。超时/错误恢复调用阻塞式 `HAL_SPI_Abort()`，恢复预算从首次错误或
超时起累计；HAL abort 返回后若总耗时已达到 10 ms，则进入 FAULTED，后续 DMA ABORT
收敛也受同一预算约束。HAL abort 本身不可中断，所以这不是 10 ms 函数返回硬截止。
恢复期间只操作 resource 已启用的 SPI/DMA IRQ。恢复失败后只有 deinit/reinit 可恢复，
不得让迟到 completion callback 污染下一事务。

H743 实际板级 pin/stream 表、PB6/PB7 CS 与 I2C1 占位解绑记录在宿主
`doc/2026-10-12-spi-dma.md`；ADC1/2/3 保留 DMA1 stream0/1/2，USART1 RX 使用
DMA2 stream6。`.ioc` 已弃用，只以 C resource、实际启动链和唯一 vector/HAL owner
作为当前配置事实，文档源码观察不能替代链接或硬件证据。

## 版本与更新

锁定子模块提交后再构建。记录宿主提交、框架提交/dirty 状态、MCU/HAL/调度 profile。
共享 framework 变更及文档进入子模块版本；宿主专属板级代码和完整硬件证据留在宿主。
维护者按既有流程提交/发布子模块，再更新宿主 gitlink，其他工程才会收到这些记录。
单纯移动未提交工作区或更新宿主文件不会使资料自动传播。

本库不提供普遍有效的构建/烧录命令；使用接入工程自己的受支持脚本，分别记录构建、编程和运行证据。
