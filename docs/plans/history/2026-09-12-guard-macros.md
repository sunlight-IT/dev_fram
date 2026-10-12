# Historical guard macro pilot

> Authority: historical source excerpt, not current API or current verification.
> Source: S004, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/plans/2026-09-12-usr-check-macro-pilot.md`.

Complete framework-only pilot plan, preserved as a proposal rather than completion evidence.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-68

<!-- SOURCE_FRAGMENT S004-F1 -->
# Guard Macro Pilot Implementation Plan

## Goal

Reduce repetitive defensive `if`/`return` blocks in the reusable GPIO and
timer device wrappers without hiding control flow or changing error behavior.

## Scope

- Add `Usr/common/usr_check.h` with explicit return-oriented guard macros.
- Migrate only the simple argument and operation-table checks in
  `Usr/drivers/bus/dev/dev_gpio.c` and `Usr/drivers/bus/dev/dev_timer.c`.
- Keep `dev_timer_get_ticks()` explicit because it returns a value rather than
  an `usr_status_t` and has a distinct invalid-value contract.
- Keep SPI transactions, UART/logging paths, DMA, MSP, IRQ/callback paths,
  cleanup, rollback, locking, and state transitions explicit.

## Macro Contract

The condition is a failure predicate: `true` means return immediately.

```c
USR_RETURN_IF(condition, return_value);
USR_RETURN_VOID_IF(condition);
USR_RETURN_STATUS_IF_NOT_OK(status_variable);
USR_RETURN_IF_LOG(condition, return_value);
USR_RETURN_VOID_IF_LOG(condition);
```

All macros use `do { ... } while (0)`. The check remains active regardless of
logging configuration. The logging variants are opt-in and are not used in
this pilot because the logging path calls back into the UART driver.

`USR_RETURN_STATUS_IF_NOT_OK` accepts an already evaluated local
`usr_status_t`, preserving the exact status returned by an operation.

## Task 1: Verify the missing macro contract

Create a compile-only macro harness and verify it fails before the header
exists.

## Task 2: Add and verify the guard header

Add `usr_check.h` and verify void returns, status propagation, single
condition evaluation, and statement-safe expansion with the ARM compiler.

## Task 3: Migrate the GPIO and timer pilot

Replace only the selected GPIO and timer wrapper guards with the macros.

## Task 4: Verify and document the pilot

Run clangd diagnostics/reference checks, `git diff --check`, and the repository
build. Record the exact results and remaining hardware status.

## Task 5: Review scope

Review the diff for layering violations and accidental migration of
cleanup/state/interrupt paths.

## Acceptance Criteria

- No universal ambiguous `CHECK(x)` macro is introduced.
- Conditions are evaluated once by the condition-based macros.
- The logging variants are present but do not affect the non-logging pilot.
- Public GPIO and timer APIs and return values are unchanged.
- No HAL/vendor include is added below `Usr/port`.
- Existing SPI/UART/MSP/DMA worktree changes remain untouched.
<!-- END_SOURCE_FRAGMENT S004-F1 -->
