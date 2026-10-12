# Framework Coding Rules

Authority: existing repository requirements scoped to dev_fram, not automatic
adoption of cross-project review drafts. Sources: host AGENTS sections 2-9 and
host `.codex/rules/firmware.md`, pinned in the source manifest.

## Language And Interfaces

- Use C11, two-space indentation and opening braces on their own lines.
- Keep modules paired as `.c/.h`; preserve established `dev_`, `usr_`, `port_hal_`
  and HAL hook names. Use fixed-width integer types for hardware-facing data.
- Keep APIs narrow, functions short and dependencies explicit.
- Use `ops + context + config`; separate immutable resource descriptions,
  mutable state and operations. Use static allocation; no heap registries or
  device-tree discovery. Add abstractions only for demonstrated shared needs.
- Runtime changes go through the public control/configuration operation.
  Upper layers must not mutate HAL handle fields.
- Use existing `usr_status_t` values at the abstraction boundary. Preserve
  busy, timeout, state and device errors; do not return raw HAL status values.
- Before changing an ops signature inspect every implementation and caller,
  including host implementations. Record compatibility and migration impact.

## Ownership

- Bus wrappers must not include vendor headers or choose physical peripherals,
  pins, IRQ numbers, DMA channels or chip-select pins.
- Adapters and the host BSP own HAL types, resources, clocks, MSP and IRQ routing.
  Protocol behavior and business policy belong to the host's higher layers.
- Chip select remains a logical GPIO resource controlled through bus operations.
- Keep DMA private to the consumer adapter and board binding. Do not introduce
  a public `dma_device_t` without a proven shared requirement.
- Preserve explicitly retained legacy platform services; never add a second
  resource owner or dispatch an owned interrupt twice.
- Keep static RAM/Flash use low. Specify asynchronous buffer lifetime and
  ISR/foreground access when changing a shared resource.
- Keep ISR work bounded. Defer parsing, logging and complex state machines.
  Use callbacks and state flags for asynchronous work.

## Working Protocol

- Read the actual call chain, data flow and ownership before changing drivers.
- Inspect Git status; preserve user changes. Keep modifications within scope.
- Inspect diffs for hardware leakage, run `git diff --check`, then the host's
  supported build. Review errors and new warnings; verify hardware-facing
  changes on the affected board. Report build, flash and runtime separately.
- Host framework tests may provide supplementary evidence; they cannot replace
  the required firmware build and hardware observations.
- Update framework changelog, AI provenance, interface/current-state documents
  and reproducible completion evidence for non-trivial changes.

Current code is not claimed to satisfy every rule. See the versioned current
state and draft gap review before treating existing patterns as recommendations.
