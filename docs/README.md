# dev_fram 文档导航

本库只覆盖 dev_fram 子模块。源码事实基线为
`d29419be990aa562bcbacabb17ac9c78b31a0908`；历史资料导入源为主工程
`8ab7a95033b30b5848def22971a35bacab766645`。后续检出不同提交时，必须重新核对源码。

2026-10-12 SPI DMA 增补来自 fresh 工作树观察：framework HEAD
`9bb4b31fd57696a34b0126505ef2ff4a27cffe0a`、H743 宿主 HEAD
`1f1b738d30b936f015eb689568ad57b97bda072d`，两者均有未提交修改。
固定基线来源记录不证明这些修改；接口与移植影响见
[SPI DMA 完成记录](completions/2026-10-12-spi-dma.md)。

2026-10-13 在同一未提交工作树上完成 SPI adapter 裸机多模式改造：去除 SPI
FreeRTOS 同步层，TX/RX 可分别选择 polling、interrupt 或 DMA，并支持单方向 DMA
资源。当前实现、验证结果与硬件待验项见
[SPI 裸机多模式完成记录](completions/2026-10-13-spi-bare-metal-modes.md)。

## 阅读入口

- [AI 工作入口](../AGENTS.md)与[现行编码规则](../rules/coding.md)。
- [当前架构](specs/current-architecture.md)、[真实公开头文件清单](specs/api-inventory.md)。
- [宿主接入契约](specs/host-integration.md)、[历史路径映射](specs/legacy-path-map.md)。
- [总线规范草案](specs/bus-guideline-draft.md)、[Port/BSP 规范草案](specs/port-bsp-guideline-draft.md)、[条款核查表](specs/draft-gap-review.md)。
- [验证证据索引](completions/evidence-index.md)、[完成记录模板](completions/TEMPLATE.md)。
- [SPI DMA 前一版工作树历史证据](completions/2026-10-12-spi-dma.md)：
  成组可选 claim/release/is_owner、3 个 SPI index、独立信号 pin、双向 DMA、
  opaque host sync ops、任务 owner、CS 多帧同步与 completion/error/timeout/abort；
  该记录保留为改造前历史证据。
- [SPI 裸机多模式当前接口与证据](completions/2026-10-13-spi-bare-metal-modes.md)：
  TX/RX 独立模式、单方向 DMA、裸机同步等待、超时恢复与 FAULTED 隔离。
- [来源清单](../scripts/docs-sources.json)、[导入选择清单](../scripts/kg200z-import.json)。

## 分类与权威

| 位置 | 内容 | 使用方式 |
| --- | --- | --- |
| `rules/` | 从现行工程要求抽取的框架规则 | 子模块工作约束，不含 KG200Z 专属配置 |
| `docs/specs/` | 当前源码快照、宿主契约和草案 | `active` 描述固定基线；`unverified` 草案待审查 |
| `docs/plans/` | 新计划和历史方案 | 表达意图，不能证明实施 |
| `docs/logs/` | 框架变更、AI 来源与历史提交 | 事实时间线，不能证明运行验证 |
| `docs/completions/` | 具体证据与未验证事项 | 区分当前文档验证和旧固件验证 |
| 各类目录的 `history/` | 已标明来源的原文摘录 | 仅解释演进，不执行其中旧命令 |

`historical` 记录当时上下文，`unverified` 表示草案或未完成核查，`active`
表示当前仍使用的文档入口。原主工程的 `implemented` 标签不自动移植到当前子模块。

## 来源与持续维护

来源清单保存原仓库、完整提交、原路径、Git blob、原始字节 SHA-256、摘录行范围、
摘录哈希和转换说明；导出包另有逐文件 SHA-256。新写文档引用源码来源记录及实测命令，
没有把聊天记忆当作实现证据。所有候选资料的纳入或排除由导入清单和来源清单记录。

SPI DMA 新文档沿用 S039/S040/S045/S052/S053/S060/S061 等固定基线的来源记录，
并明确区分本次 fresh 源码阅读和主 agent 提供的测试/构建结果。
未提交源码没有被冒充为固定 commit 的 Git blob 或 SHA-256。

本次没有打包芯片协议、facade、应用业务、真实板级支持源码、固件测试脚本、固件产物或本机 AI 配置。
历史摘录中仍可能提到宿主或设备，这是解释原有资源边界所需的上下文，不是移植代码或设备规范。

从子模块根目录维护与导出：

文档工具仅依赖 Python 3.10+ 标准库；PowerShell wrapper 需要 `python` 在 PATH。

```powershell
python scripts/docs-library.py update
python scripts/docs-library.py check
python tests/docs_library_test.py
python scripts/docs-library.py export
```

也可使用 `powershell -ExecutionPolicy Bypass -File scripts/docs-index.ps1 -Update` 和 `-Check`。
默认导出到被忽略的 `exports/`，ZIP 内含文档、来源索引及文档工具，不含源码文件。
