# Documentation governance provenance

> Authority: historical source excerpt, not current API or current verification.
> Source: S016, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/completions/2026-09-29-documentation-taxonomy-and-index.md`.

Document taxonomy and historical evidence boundary; excluded old host check counts.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 3-16

<!-- SOURCE_FRAGMENT S016-F1 -->
## 任务范围

本次任务只处理项目文档分类、文档索引、Codex 规则和文档路径管理脚本。
未修改 STM32 固件源代码、构建配置或硬件资源绑定。

完成的分类如下：

- `docs/logs/`：活动日志和项目变更日志。
- `docs/plans/`：任务计划和构建烧录工作流。
- `docs/specs/`：单次设计规范。
- `docs/completions/`：完成记录和模板。

旧 `docs/superpowers/` 下的计划和规范使用 Git 重命名迁移，历史内容没有被合并或重排。
`.superpowers/sdd/` 保持为被忽略的本地过程资料，没有被导入正式文档目录。
<!-- END_SOURCE_FRAGMENT S016-F1 -->

## Source lines 74-83

<!-- SOURCE_FRAGMENT S016-F2 -->
## 剩余风险

- 历史日志和历史计划正文中仍可能出现旧路径或旧命令，它们保留为历史语境，不能作为当前操作指令。
- 文档元数据需要在新增、删除、移动、重命名或分类变化时同步更新；`docs-index.ps1 -Check` 会检查文件集合和索引哈希，但不会判断描述文字是否符合任务意图。
- `scripts/docs-index.ps1` 使用 UTF-8 读取元数据和 `AGENTS.md`，依赖 Windows PowerShell 5.1 或 PowerShell 7 的兼容行为；两种入口均已实际检查。

## 后续动作

后续每次文档变更都必须先更新 `scripts/docs-metadata.json`（如文件集合或分类变化），再运行索引更新和检查；完成固件任务时另行更新项目变更日志并记录独立的构建、烧录和硬件证据。

<!-- END_SOURCE_FRAGMENT S016-F2 -->
