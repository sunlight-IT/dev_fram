# dev_fram Change Log

## 2026-10-13 - SPI bare-metal transfer modes

- Added independent TX/RX polling, interrupt and DMA mode configuration.
- Removed SPI FreeRTOS synchronization hooks and host sync module; retained a
  bare-metal non-recursive claim and synchronous foreground API.
- Allowed per-direction DMA resources and added mixed-mode full-duplex polling
  fallback, shared two-stage timeout and guarded abort/FAULTED recovery.
- Replaced the host synchronization test with H743 DMA memory-policy tests and
  expanded SPI host coverage across the complete mode matrix and recovery paths.

## 2026-10-12 - SPI DMA working-tree documentation

- Recorded fresh, uncommitted SPI changes at framework HEAD
  `9bb4b31fd57696a34b0126505ef2ff4a27cffe0a`: optional grouped
  claim/release/is_owner, 3 indices, independent SCK/MISO/MOSI pin resources,
  RX/TX DMA descriptors and opaque host synchronization operations.
- Documented task ownership and multi-frame CS, synchronous return over DMA,
  byte-to-frame conversion for 8/16 bit, completion/error/timeout/abort,
  buffer lifetime, HAL EOT ownership and H743 DTCM/D-Cache rejection.
- Added framework and host evidence records and baseline provenance references.
  This documentation side task changed no C/H/Python/CMake code and made no commit.
  Test/build results supplied by the main agent are separate from fresh document
  checks. Debug/Release builds and clangd checks are recorded; the final
  diagnostics-off flash and complete target runtime acceptance remain pending.

## 2026-10-12 - CAN/FDCAN device framework

- Added HAL-independent Classic CAN/CAN FD frame, config, status and
  `init/deinit/send/recv/get_status` operations.
- Added STM32 FDCAN resource/instance adapter with polling or interrupt receive,
  complete-frame SPSC queueing and FIFO0/FIFO1/dedicated-buffer routing.
- Moved FDCAN mode, Message RAM layout, element sizes, TX FIFO/Queue mode,
  filter destination and RX location into immutable board resources.
- Added host API tests, resource-wiring checks and CRLF-stable documentation
  metadata hashing. Firmware and hardware evidence remain host-owned.

## 2026-10-10 - Portable AI documentation library

- Added framework-local AI instructions, scoped existing rules, two review-draft
  guidelines, current API/source mapping and host integration boundaries.
- Imported selected pinned historical plans/specs/logs/completion excerpts;
  kept removed capture/claims and changed PWM channel values historical.
- Added source hashes, line ranges, extraction transformations and document
  inventory/check/export tools. The only added tests exercise these tools.
- No firmware source, API, board binding, build input or hardware behavior changed.
- Exact document/package verification is recorded in the associated completion.

Earlier source changes are in `history/framework-git-history.md`. Imported host
history is in `history/kg200z-framework-changelog.md` and is not current validation.
