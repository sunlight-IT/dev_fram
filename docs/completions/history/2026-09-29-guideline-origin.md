# Guideline provenance excerpts

> Authority: historical source excerpt, not current API or current verification.
> Source: S015, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/completions/2026-09-29-cross-project-guidelines.md`.

Review considerations and draft authority only. Does not import facade rules or claim framework compliance.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 40-50

<!-- SOURCE_FRAGMENT S015-F1 -->
## 审查依据

独立子 agent 对输出计划进行了审查，重点指出并已纳入：

- 当前实现、目标架构和跨工程规则必须分开。
- `Usr/app` 组合根和 Core/legacy 边界必须明确。
- 当前单例 facade、具体 driver 指针、`ops + ctx`、静态分配和 `process()` 只能作为映射或推荐模式，不能全部写成跨工程 MUST。
- 必须定义 callback 执行上下文、DMA buffer lifetime、锁、内存屏障和 deinit 顺序。
- 必须区分 BUSY、异步请求已接受、NOT_READY、NO_DATA 和操作完成。
- 必须覆盖无 HAL、RTOS、无 DMA、无缓存和无硬件 timeout 等平台 profile。

<!-- END_SOURCE_FRAGMENT S015-F1 -->

## Source lines 98-107

<!-- SOURCE_FRAGMENT S015-F2 -->
## 剩余风险

- 规范仍处于 `Draft for review`，需要用户审查后才能决定是否批准、拆分或纳入更高优先级规则。
- 规范检查脚本可以验证结构、规则 ID、路径和关键边界，但不能替代人工判断规范是否适合某个具体 RTOS、MCU 或产品安全等级。
- 当前项目映射会随源码变化，需要在后续规范或架构变更中重新核对。

## 后续动作

用户审查通过前，保持三份文档为 `Draft for review` 和 metadata 状态 `unverified`。批准后再决定是否修改状态、抽取到 `.codex/rules/`，或按不同平台拆分 profile。

<!-- END_SOURCE_FRAGMENT S015-F2 -->
