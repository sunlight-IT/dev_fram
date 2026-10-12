# 宿主工程接入契约

基线与来源：framework `usr_port.h`、`usr_port_hal.h`、`board_support/demo/*`、
`port_bsp/*.h`、`port_hal_tim.c`、`log/my_log.h`，详见来源清单。

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
- BSP 定义实际 resource 表、instance 存储、设备 `ctx/ops` 绑定，以及 UART/SPI/I2C/TIM/PWM 的 instance/DMA 查询。
- GPIO/I2C/time 查询与 platform init/system reset 等入口按实际公开声明由宿主实现；不要照抄 demo 中不匹配的 timer 声明。
- 使用 logger 时提供 `zlog`、level、tick、输出等实现；只存在 `my_log.h` 不能证明日志已可用。

## IRQ、MSP、时钟与 DMA

宿主为每个启用外设明确一个 IRQ/vector/MSP 所有者，核查哪些 HAL hooks 已由 adapter 强定义。
UART 物理 IRQ 经当前 `port_hal_uart_irq(handle)` 接入；DMA callbacks 和错误 callbacks
按返回 bool 的 ownership 协议分发。不得同时分发 port 和 legacy owner。

当前 TIM 仍强定义 TIM2 vector 和 period callback，因此换板或使用 HAL TIM timebase 前须检查
重复符号、资源查询、未绑定 handle 和 `SYSTEM_TIMER` 的覆盖范围。这个问题是接入限制，
本次文档整理未解决它。

宿主必须确认 DMA channel/request、storage 寿命、缓存/对齐、IRQ 延迟和 ring 消费时序。
DMA 半缓冲 commit 要求偶数容量且提交长度为容量一半；完整接收行为需目标板观察。

## 版本与更新

锁定子模块提交后再构建。记录宿主提交、框架提交/dirty 状态、MCU/HAL/调度 profile。
共享 framework 变更及文档进入子模块版本；宿主专属板级代码和完整硬件证据留在宿主。
维护者按既有流程提交/发布子模块，再更新宿主 gitlink，其他工程才会收到这些记录。
单纯移动未提交工作区或更新宿主文件不会使资料自动传播。

本库不提供普遍有效的构建/烧录命令；使用接入工程自己的受支持脚本，分别记录构建、编程和运行证据。
