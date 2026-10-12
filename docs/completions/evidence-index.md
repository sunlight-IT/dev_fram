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

原主工程日志中的设备/CLI运行数据不是此次子模块文档库的验证范围，未整体导入。
没有原始日志文件或准确 image/commit 的结论维持“历史文档报告”，不声称已独立复核。

后续每项硬件证据至少注明宿主和框架完整 commit、dirty 状态、平台/HAL、执行命令、
输出、观察条件、未验证事项；变更共享框架后的宿主验证不能覆盖其他 MCU/profile。
