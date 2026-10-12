# dev_fram Change Log

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
