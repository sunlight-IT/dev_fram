# 框架验证证据索引

这里只整理来源记录；本次没有复跑历史固件构建、烧录或硬件观察。
源文件快照提交是“记录所在版本”，不一定是记录中被测试的源码版本。
若文档没有准确给出被测 commit/image，就标为版本未确定。

| 记录 | 原文中的证据 | 适用范围与缺口 |
| --- | --- | --- |
| [语义 TIM/PWM](history/2026-09-30-semantic-timer-pwm.md) | 25 项资源检查、语义与生命周期测试、ARM build 0 errors/119 warnings | 历史语义架构；被测精确提交未确定；硬件未验证，部分功能后删除 |
| [精简 TIM/PWM](history/2026-10-09-compact-timer-pwm.md) | 结构/静态/生命周期测试，build 0 errors/148 warnings | 10 月 9 日共享 adapter 版本；不覆盖后来 TIM/PWM 分离 |
| [多通道 PWM](history/2026-10-09-multichannel-pwm.md) | 结构/生命周期检查，build 0 errors/77 warnings | 当时通道 1..4；当前 enum 为 0..3；未做真实 PWM waveform |
| [框架历史日志](../logs/history/kg200z-framework-changelog.md) | UART/DMA/ring、MSP、宏试点等命令和结果，也保留失败/待验证信息 | 记录版本未完全确定；不要把相邻条目的结果归给不同固件 |
| 同日志中的 9 月 24 日资源实例统一记录 | ring/12 项结构检查、构建和编程报告，以及 DMA/ring/IRQ 观察 | 从原条目选取框架相关片段，CLI和设备状态未导入；被测精确版本未确定 |
| [AI 历史日志](../logs/history/kg200z-framework-ai-log.md) | 对应 AI 工作来源、检查限制和历史失败 | 有重复/混合条目；保留事实，不补造缺失归属 |
| [Git 演进](../logs/history/framework-git-history.md) | 提取、BSP 移出、TIM/PWM 分离、timebase/IRQ 提交 | Git 提交只能证明源码演进，不能证明编译或运行 |
| [本次文档完成记录](2026-10-10-portable-document-library.md) | 当前文档库与 ZIP 的实测命令/结果 | 文档验收，不是固件证据 |
| [CAN/FDCAN 框架](2026-10-12-can-device-framework.md) | 公共 API 主机测试、resource 路由测试、H743 Debug/Release 构建和链接符号 | 未烧录；未验证真实 CAN 收发、过滤、ACK、bus-off 或 CAN FD 波形 |
| [SPI DMA 工作树](2026-10-12-spi-dma.md) | dev_spi/port_hal_spi/spi_sync host tests PASS；Debug FLASH 218752 B/RAM 45912 B；Release FLASH 118424 B/RAM 45864 B；clangd 3 个目标文件 0 errors；文档库检查与测试见完成记录 | framework `9bb4b31`、host `1f1b738` 均 dirty；最终正常镜像因调试器 `open failed` 未确认烧录；诊断镜像仅观察到有限 SPI4 EOT/timeout recovery；FRAM RDID 全零、SPI2/3 无物理回环，真实协议 runtime 未通过验收 |
| [SPI 裸机多模式工作树](2026-10-13-spi-bare-metal-modes.md) | TX/RX 独立 polling/IT/DMA、单方向 DMA、3x3 全双工矩阵、裸机 timeout/abort/FAULTED host tests；Debug/Release、clangd、文档与 diff 检查 | framework/host 均 dirty；未烧录，目标板 SCK/MOSI、FRAM、CS 和恢复行为由用户验证 |

原主工程日志中的设备/CLI运行数据不是此次子模块文档库的验证范围，未整体导入。
没有原始日志文件或准确 image/commit 的结论维持“历史文档报告”，不声称已独立复核。

SPI DMA 的当前 CAN host test 失败是本次交接信息，不改写上表中的旧 CAN 历史结果，
也不将旧 Release/链接/运行结论转移到当前 SPI DMA 工作树。

后续每项硬件证据至少注明宿主和框架完整 commit、dirty 状态、平台/HAL、执行命令、
输出、观察条件、未验证事项；变更共享框架后的宿主验证不能覆盖其他 MCU/profile。
