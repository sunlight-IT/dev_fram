# Framework AI Provenance

## 2026-10-13 - SPI bare-metal multi-mode implementation

- Agent: Codex in the shared dirty H743 checkout; followed the user-approved
  implementation plan and preserved unrelated host/framework changes.
- Added per-direction SPI transfer modes, independent DMA lifecycle and a
  bare-metal synchronous state machine with context checks, timeout, abort and
  FAULTED quarantine. Removed the SPI FreeRTOS sync module and test stubs.
- Used test-driven host changes, including observed failing tests for the new
  config fields, resource callback/state additions, context rejection and the
  timeout-before-abort callback race before implementing fixes.
- Updated current documentation without rewriting the 2026-10-12 record, which
  remains historical evidence for the superseded RTOS synchronization design.
- Did not flash hardware, commit or push. A read-only subagent review and final
  verification commands are recorded by their actual outcomes before handoff.

## 2026-10-12 - SPI DMA documentation side task

- Agent: Codex, document-only side task in the shared H743 checkout; no child
  agent, production/test source edits, commit or push.
- Observed framework HEAD: `9bb4b31fd57696a34b0126505ef2ff4a27cffe0a`, dirty.
  Observed host HEAD: `1f1b738d30b936f015eb689568ad57b97bda072d`, dirty.
- Read root/framework AGENTS, coding/documentation rules and current documents;
  used CodeGraph before source search, then read changed and unindexed source.
  Current SPI contracts are fresh working-tree observations, not pinned blobs.
- Kept S039/S040/S045/S052/S053/S060/S061 and historical fragments unchanged;
  registered the new authored completion against baseline references with an
  explicit fresh-observation transformation. Original API excerpts remain intact.
- Recorded current-worktree SPI host tests, Debug/Release memory sizes and clangd
  results. Host tests pass; clangd reports 0 errors for `main.c`, the SPI adapter
  and SPI IRQ file, while the `usr_spi_sync.c` code-action tool reports a
  replacement-overlap diagnostic rather than a compile error. Existing CAN
  nominal-timing assertion failure remains outside this change.
- The final diagnostics-off image was not confirmed programmed because ST-Link
  open failed. A temporary diagnostics-on image was programmed successfully and
  gave limited SPI4 EOT/timeout-recovery observations, but FRAM RDID returned
  `USR_ERR_DATA` with an all-zero buffer and SPI2/SPI3 had no physical loopback.
  Therefore the completion record and host board record keep programming,
  runtime, and unverified scope separate.

## 2026-10-12 - CAN/FDCAN device framework

- Agent: Codex in the shared H743 checkout; implementation followed an approved
  design and an independent read-only subagent review.
- Initial framework: `bb9c659fff03de48d35682f949bbed8b31e5473c`, clean.
- Initial host: `68d328f20128e9d94beb95033c9f608f167f8264`, with an existing
  `Core/Src/fdcan.c` data-timing edit that was preserved and incorporated.
- Added portable CAN API, STM32 FDCAN adapter, demo getter, H743 resource binding,
  fake-ops tests and resource-ownership checks. The host `.ioc` was explicitly
  excluded because the project no longer uses it.
- Verification and unverified hardware scope are recorded in
  `docs/completions/2026-10-12-can-device-framework.md`.

## 2026-10-10 - Portable documentation library

- Agent: Codex in the shared checkout; approved plan executed without commit/push.
- Initial framework: `d29419be990aa562bcbacabb17ac9c78b31a0908`, clean.
- Initial host: `8ab7a95033b30b5848def22971a35bacab766645`, clean.
- Sources: local Git objects, current source, existing AGENTS/rules and indexed
  historical documentation. Source IDs, file/blob hashes and selections are
  registered in `scripts/docs-sources.json` and `scripts/kg200z-import.json`.
- Output: AI entry, current architecture/API, scoped rules/drafts, selected
  history, evidence mapping and document checks/export tools.
- Verification: exact fresh results are in the completion record. Negative
  document-tool tests were written and failed before the tool existed.
- Limitation: no firmware build, flash, runtime observation or independent
  reviewer subagent was available/performed in this document task. Self-review
  and executable document-tool acceptance are reported separately.
