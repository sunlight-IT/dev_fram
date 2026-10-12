# Imported framework AI provenance

> Authority: historical source excerpt, not current API or current verification.
> Source: S018, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/logs/ai-generation-log.md`.

Selected framework work and failures. Mixed/duplicated historical entries retained without inventing their missing attribution; device status and CLI runtime entries excluded.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 3-105

<!-- SOURCE_FRAGMENT S018-F1 -->
## 2026-10-09 - Multi-channel timer PWM output

- Agent: Codex, implementation performed in the shared working tree.
- Summary: added logical PWM channels 1..4, static per-channel GPIO/AF,
  polarity and CCR resources, channel-specific compare updates, group
  start/stop, and rollback handling. No unverified board PWM mapping was
  added.
- Verification: host lifecycle tests, static contract tests, resource
  structure checks, and `python .\scripts\build.py` passed with `0 errors,
  77 warnings`; flash, register, RTT, and waveform checks remain outstanding.

## 2026-10-09 - Compact static TIM/PWM port adapter

- Agent: Codex, implementation performed in the shared working tree.
- Basis: `AGENTS.md`, `.codex/rules/firmware.md`, the reviewed compact TIM/PWM
  plan, current `Usr/hal` sources, and the TIM2 scheduler call chain.
- Summary: removed capture and owner/claim/quantization layers; changed timer
  and PWM facades to static board-bound configuration; merged PWM operations into
  the TIM port instance; kept TIM2 as the 1 ms scheduler.
- Verification: compact TIM/PWM lifecycle and static contract tests pass; an ARM
  build completed with zero errors. The legacy bus fixture still assumes old
  UART/SPI types and is not evidence for this timer-only change.
- Remaining risk: no target flash, RTT, timer register, or PWM waveform test was
  performed; the current board has no confirmed PWM pin resource.

## 2026-10-02 - Semantic timer, capture, and PWM resource architecture

- Agent: Codex, implementation performed in the shared working tree; a fresh
  read-only subagent review is required before final handoff.
- Basis: `AGENTS.md`, `.codex/rules/firmware.md`, the application/driver/port
  call chains, the STM32WLE5 HAL interfaces already used by this repository,
  and `docs/plans/2026-09-30-semantic-timer-pwm-resource-architecture.md`.
- Summary: migrated UART/SPI/TIM/PWM configuration to semantic application
  contracts; added static TIM resource ownership and lifecycle handling; added
  basic timer, capture ring/IRQ dispatch, and TIM-backed PWM support; preserved
  TIM2 scheduler ownership and kept unverified PWM/capture resources disabled.
  Port adapters retain claims in a fault state after failed cleanup and allow
  explicit `deinit` cleanup retry. SPI 16-bit transfers validate byte length
  and alignment before converting byte counts to HAL frame counts.
- Files: `Usr/drivers/bus/dev/dev_uart.[ch]`,
  `Usr/drivers/bus/dev/dev_spi.[ch]`, `Usr/drivers/bus/dev/dev_timer.[ch]`,
  `Usr/drivers/bus/dev/dev_pwm.[ch]`, `Usr/drivers/bus/dev/dri_ops.h`,
  `Usr/port/board_support.[ch]`, `Usr/port/port_bsp/port_hal_uart.[ch]`,
  `Usr/port/port_bsp/port_hal_spi.[ch]`,
  `Usr/port/port_bsp/port_hal_tim.[ch]`,
  `Usr/port/port_bsp/port_hal_pwm.[ch]`,
  `Usr/port/port_bsp/port_tim_semantic.[ch]`, `Usr/app/usr_app.c`,
  `Usr/app/usr_app_config.h`, `CMakeLists.txt`, and the host test scripts.
- Verification so far: `git diff --check` exited `0`; all five host test
  scripts passed; `python .\\scripts\\build.py` exited `0` with
  `[BUILD] SUCCESS (0 errors, 119 warnings)` and produced non-empty ELF/HEX/BIN
  artifacts. The fresh build size is `text=258364`, `data=2004`, `bss=18044`;
  Flash load is `260368` bytes and remaining Flash is `1776` bytes. The
  warning set contains repeated existing RTC macro redefinitions, existing
  unused/return/type/format diagnostics, and the existing linker RWX LOAD
  segment warning.
- Hardware status: flash, RTT, and TIM2 register evidence are pending this
  run. Physical PWM/capture/BOTH-edge behavior is intentionally unverified.
- Remaining risks: no live PWM/capture board row exists; BOTH-edge edge
  classification relies on GPIO IDR sampling in the capture ISR; the current
  build has `1776` bytes of remaining Flash and the new semantic peripheral
  sources use local `-Os` properties. `conflict_domain` is board metadata only;
  the single live TIM2 row does not exercise cross-descriptor arbitration.

## 2026-09-29 - Cross-project three-layer coding guideline drafts

- Agent: Codex
- Summary: Added draft cross-project guidelines for the reusable driver layer, port/BSP layer, and application-facing device facade. The documents separate normative rules from the current KG200Z implementation mapping and remain outside `.codex/rules/` pending user review.
- Files: `docs/specs/2026-09-29-cross-project-device-driver-guideline.md`, `docs/specs/2026-09-29-cross-project-port-bsp-guideline.md`, `docs/specs/2026-09-29-cross-project-device-facade-guideline.md`, `scripts/docs-guideline-check.ps1`, `scripts/check.ps1`, `scripts/docs-metadata.json`, `docs/README.md`, `AGENTS.md`, `docs/completions/2026-09-29-cross-project-guidelines.md`
- Verification: `docs-guideline-check.ps1` passed with 3 guideline documents and 61 unique rules; metadata JSON parsed with 19 entries; PowerShell parser checks passed for the four changed scripts; `scripts/check.ps1`, docs index checks, current-path scans, and `git diff --check` passed; firmware build, flash, and hardware validation were not run.
- Follow-up: User review is required before changing guideline status from `unverified` or promoting any rule into `.codex/rules/`.

## 2026-09-29 - Documentation taxonomy and generated index

- Agent: Codex
- Summary: Reclassified the tracked documentation into logs, plans, specs, and completions; added a generated `AGENTS.md` index, documentation metadata, maintenance rules, and current Python build/flash entry points.
- Files: `AGENTS.md`, `.codex/README.md`, `.codex/rules/documentation.md`, `.codex/rules/firmware.md`, `.codex/rules/git.md`, `.codex/rules/project_rules.md`, `docs/README.md`, `docs/logs/CHANGELOG.md`, `docs/logs/ai-generation-log.md`, `docs/plans/`, `docs/specs/`, `docs/completions/TEMPLATE.md`, `scripts/check.ps1`, `scripts/docs-index.ps1`, `scripts/docs-metadata.json`, `scripts/record-generation.ps1`
- Verification: `powershell -ExecutionPolicy Bypass -File .\scripts\docs-index.ps1 -Update` and `-Check` passed for 15 documentation files; `pwsh -NoProfile -File .\scripts\docs-index.ps1 -Check` passed; PowerShell parsing passed for `scripts/docs-index.ps1`, `scripts/check.ps1`, and `scripts/record-generation.ps1`; the stale-index negative test returned exit code 1 for an unregistered file and cleaned up the probe; `scripts/check.ps1` and `git diff --check` passed.
- Follow-up: Firmware build, flash programming, serial/RTT observation, and peripheral runtime validation were not run because this task changed documentation, rules, and path-management scripts only.

## 2026-09-24 - UART/SPI/I2C single-instance resource consolidation

- Agent: Codex
- Summary: Consolidated UART, SPI, and I2C board resources and mutable runtime
  state into one strongly typed instance per physical peripheral. Board
  support owns resource tables and instances; port adapters consume them.
  Existing application-facing bus APIs were preserved.
- Files: `Core/Src/usart.c`, `Core/Src/spi.c`, `Core/Src/i2c.c`,
  `Usr/drivers/bus/dev/dev_uart.h`, `Usr/port/board_support.c`,
  `Usr/port/port_bsp/port_hal_uart.[ch]`,
  `Usr/port/port_bsp/port_hal_spi.[ch]`,
  `Usr/port/port_bsp/port_hal_i2c.[ch]`, `Usr/port/usr_port_hal.h`,
  `scripts/peripheral_resource_structure_test.py`,
  `scripts/ring_buf_host_test.py`, and `tests/ring_buf_host_test.c`.
- Host/static verification: the diagnostic Ring Host test passed; resource
  ownership checks passed 12/12; `git diff --check` passed with local
  permission and line-ending notices only.
- Build verification: the final project build reported
  `[BUILD] SUCCESS (0 errors, 0 warnings)` and produced non-empty ELF, HEX,
  and BIN artifacts. Static RAM decreased by 8 bytes compared with the
  pre-refactor Ring baseline.
- Flash verification: the repository flash script reported `Verified OK`,
  target reset, and `[FLASH] SUCCESS`.
<!-- END_SOURCE_FRAGMENT S018-F1 -->

## Source lines 118-185

<!-- SOURCE_FRAGMENT S018-F2 -->
## 2026-09-24 - Direct DMA-to-ring revision

- Agent: Codex
- Summary: Removed the UART DMA staging buffer. The USART1 circular DMA
  target is now the unified ring buffer itself; HT/TC callbacks commit
  half-buffer ownership without copying. The ring uses read/write positions
  plus epochs, while USART2 keeps software/IT writes. UART NVIC enablement is
  explicit in board RX contexts: USART1 DMA error IRQ enabled, USART2 IT IRQ
  enabled, and LPUART1 NONE IRQ disabled. The stray HAL UART `;W` token was
  removed.
- Verification: ARM ring syntax check passed; structural checks passed 11/11;
  project build passed with 0 errors and 2 existing warnings. Final flash and
  runtime validation for this direct-DMA revision remain pending.
- Final flash: project flash script reported `Verified OK`, `Resetting Target`,
  and `[FLASH] SUCCESS`. The external USB-UART port re-enumerated from [historical serial port] to
  [historical serial port]/[historical serial port] after reset and neither new port answered the CLI probe, so no
  direct-DMA runtime CLI claim is made for this final image.
- Reconnected final verification: [historical serial port] returned `ver` and `param show`;
  MKLink [historical serial port] connected with IDCODE `0x6BA02477`. Raw reads showed DMA1
  Channel2 enabled with `CMAR=0x20002F2C`, the unified USART1 ring buffer,
  non-zero changing `CNDTR`, and USART1 `CR3=0x41`. RX contexts showed
  USART1 DMA/IRQ enabled, USART2 IT/IRQ enabled, and LPUART1 NONE. Ring
  write position/epoch advanced across two snapshots, and raw CFSR/HFSR bytes
  were all zero. The observed ring overflow is expected under the documented
  half-buffer safe-window model and is recorded rather than hidden.

This file records AI-generated or AI-maintained project work. It is separate
from `docs/change_log/CHANGELOG.md`, which records firmware and architecture
changes for the project.

## 2026-09-24 - Unified ring buffer and UART DMA/IT reception

- Agent: Codex
- Commit: working tree after checkpoint `fc5539b` on `dev`.
- Summary: Replaced the two project-owned ring implementations with one
  `ring_buf_t` position/epoch ring. USART1/GPS now receives directly into the
  ring buffer through circular DMA half/full commits without a staging copy;
  USART2/CLI uses one-byte interrupt reception and the same ring read API;
  LPUART1 RX is disabled. LoRaWAN receive storage now uses the same software
  ring. UART DMA MSP ownership is restricted to USART1 Channel2.
- Files: `Usr/component/ring_buf.[ch]`,
  `Usr/port/port_bsp/port_hal_uart.[ch]`, `Usr/port/board_support.c`,
  `Usr/port/usr_port_hal.h`, `Core/Src/usart_if.c`,
  `Usr/drivers/dev/dev_lorawan.c`, and
  `scripts/peripheral_resource_structure_test.py`.
- Verification: structural checks passed 11/11; ARM C syntax-only check passed
  for the ring source and host test; `python .\\scripts\\build.py` passed with
  0 errors and 4 existing warnings and generated non-empty ELF/HEX/BIN;
  `git diff --check` passed with line-ending and Git ignore permission
  warnings only.
- Review follow-up: corrected RX `enabled` publication ordering, preserved
  committed ring bytes during deferred recovery, isolated NONE-mode and legacy
  LPUART callbacks, added the ring read-side barrier, and added
  `dma_missed_cycles` detection for delayed half/full DMA callbacks.
- Hardware: Flash and runtime verification were not yet performed for this
  ring change. The host GCC executable test could not be run because the local
  host compiler process did not return within 30 seconds. Remaining runtime
  checks are USART2 IT CLI input, USART1 DMA half/full progress, overflow under
  sustained GPS traffic, and CFSR/HFSR inspection.
- Final verification: after review fixes, structural checks remained 11/11 and
  the final `python .\\scripts\\build.py` reported 0 errors and 0 warnings.
  The final `python .\\scripts\\flash.py` reported `Verified OK`,
  `Resetting Target`, and `[FLASH] SUCCESS`. On the final image, [historical serial port]
  `ver` and `param show` returned valid responses, confirming USART2 IT CLI
  operation. MKLink 0.1.6 RAM/register reads were invalid fixed-pattern
  responses and OpenOCD read-only did not return memory words, so USART1 DMA
  counters and SCB fault registers remain unreported rather than inferred.

<!-- END_SOURCE_FRAGMENT S018-F2 -->

## Source lines 186-231

<!-- SOURCE_FRAGMENT S018-F3 -->
## 2026-09-22 - Shared DMA descriptors and clock callback generation

## 2026-09-24 - Direct DMA-to-ring revision

- Agent: Codex
- Summary: Removed the UART DMA staging buffer. The USART1 circular DMA target
  is now the unified ring buffer itself; HT/TC callbacks commit half-buffer
  ownership without copying. The ring uses read/write positions plus epochs,
  while USART2 keeps software/IT writes. UART NVIC enablement is explicit in
  board RX contexts: USART1 DMA error IRQ enabled, USART2 IT IRQ enabled, and
  LPUART1 NONE IRQ disabled. The stray `;W` token in the HAL UART source was
  removed.
- Verification: ARM ring syntax check passed; structural checks passed 11/11;
  project build passed with 0 errors and 2 existing warnings. Final flash and
  runtime validation for this direct-DMA revision remain pending.

- Agent: Codex
- Commit: working tree on `dev` (branch creation was blocked by repository
  ref-directory permissions; no existing changes were reset).
- Summary: Added a private port DMA descriptor/helper shared by UART, SPI, and
  I2C resources. UART RX DMA remains on DMA1 Channels 2, 3, and 6; ADC remains
  on Channel1. SPI1/SPI2/I2C1 now contain disabled DMA resource slots and
  conditional MSP init/deinit paths without changing polling APIs. Added
  token-paste clock callback macros with explicit HAL kernel-clock fields and
  removed the global MX_DMA initializer dependency.
- Files: `CMakeLists.txt`, `Core/Inc/dma.h`, `Core/Src/adc.c`,
  `Core/Src/adc_if.c`, `Core/Src/dma.c`, `Core/Src/stm32wlxx_it.c`,
  `Core/Src/usart_if.c`, `Usr/port/usr_port_platform.c`,
  `Usr/port/port_bsp/port_hal_dma.[ch]`,
  `Usr/port/port_bsp/port_hal_clock.h`, UART/SPI/I2C port sources,
  `scripts/peripheral_resource_structure_test.py`, and the changelog.
- Verification: `python scripts/peripheral_resource_structure_test.py all`
  passed 9 checks; `python scripts/build.py` passed with 0 errors and 22
  existing warnings, generating non-empty ELF/HEX/BIN artifacts; and
  `git diff --check` passed with only existing line-ending and Git ignore
  permission warnings. `python scripts/flash.py` completed with
  `Verified OK`, `Resetting Target`, and `[FLASH] SUCCESS` on the connected
  STM32WLE/WL5x target.
- Review follow-up: DMA IRQ dispatch now uses a single-owner boolean chain for
  UART/SPI/I2C, invalid descriptors are rejected before HAL/NVIC use, DMA IRQ
  pending state is cleared during disable, and ADC initialization returns after
  a failed DMA init.
- Runtime follow-up: programming and reset were verified; GPS RX DMA, ADC DMA,
  trace, SPI/I2C polling, and deinit/reinit IRQ behavior still need live
  peripheral observation.

<!-- END_SOURCE_FRAGMENT S018-F3 -->

## Source lines 275-392

<!-- SOURCE_FRAGMENT S018-F4 -->
## 2026-09-14 - UART DMA IRQ dispatch and MSP NVIC ownership

- Agent: Codex
- Commit: working tree
- Summary: Continued the UART MSP/NVIC decoupling task after identifying a
  hardcoded `DMA1_Channel3` conflict check tied to `s_usart2_rx_dma` and
  `s_lpuart1_rx_dma`. UART resources now carry their peripheral and RX-DMA IRQ
  metadata; MSP init/deinit owns per-resource NVIC lifecycle; DMA IRQ handlers
  dispatch by the active `IRQn_Type` through the resource table. LPUART1 RX was
  moved to DMA1 Channel 6 to keep ADC on Channel 1 and uses PB10/PB11.
- Files: `Usr/port/port_bsp/port_hal_uart.c`,
  `Usr/port/port_bsp/port_hal_uart.h`, `Usr/port/usr_port_hal.h`,
  `Core/Src/stm32wlxx_it.c`, `Core/Inc/stm32wlxx_it.h`,
  `Core/Src/usart_if.c`, `tmp/msp_nvic_ownership_test.py`,
  `docs/change_log/CHANGELOG.md`, and this log.
- Verification: `python .\\tmp\\msp_nvic_ownership_test.py` exited 0 with
  `PASS: 12 MSP/NVIC ownership checks`; the targeted
  `git diff --check` exited 0; the source scan found no old hardcoded Channel 3
  conflict predicate or old UART-handle DMA IRQ API; and
  `python .\\scripts\\build.py` exited 0 with `[100%] Built target
  KG200Z_OPEN` and `[BUILD] SUCCESS (0 errors, 16 warnings)`. The warnings
  were existing RTC macro redefinitions and a linker RWX-segment warning.
- Hardware: Not performed. Validate GPS USART1 RX, USART2 RX, LPUART1 RX,
  ADC DMA, IRQ callbacks, and LPUART1 PB10/PB11 connectivity on the board.

## 2026-09-12 - GPIO/timer guard macro pilot

- Agent: Codex with independent subagent implementation/review.
- Commit: working tree.
- Summary: Added explicit return-oriented guard macros in
  `Usr/common/usr_check.h` and migrated only the simple validation guards in
  `dev_gpio.c` and `dev_timer.c`. The pilot deliberately excludes SPI/UART,
  DMA, MSP, IRQ/callback, cleanup, rollback, lock, and state-transition paths.
  Silent macros are used at all pilot call sites to avoid log-path recursion.
- Files: `Usr/common/usr_check.h`, `Usr/drivers/bus/dev/dev_gpio.c`,
  `Usr/drivers/bus/dev/dev_timer.c`, `docs/change_log/CHANGELOG.md`,
  `docs/ai-generation-log.md`, and the implementation plan. A temporary
  harness under `tmp/` was used for verification and removed before handoff.
- Verification: ARM GCC strict compile-only macro harness exited 0; fresh
  MSYS2 UCRT GCC compilation and native harness execution both exited 0. Both
  builds compiled the real GPIO/timer wrappers; the native build also linked
  and executed them against fake operations. Runtime checks covered all five
  macros and enabled/disabled logging behavior. Fresh clangd diagnostics
  reported 0 errors/0 warnings for all three production files; strict ARM
  compilation of the final GPIO/timer sources and the tracked scoped
  `git diff --check` both exited 0. An explicit trailing-whitespace scan of
  the two untracked pilot files found no matches. Full `git diff --check`
  exited 2 on unrelated trailing whitespace in `port_hal_uart.h`. Final
  `python .\scripts\build.py` exited 1 after the pilot sources compiled,
  blocked by concurrent UART edits that redefine `usr_port_hal_uart_resource_t`
  and reference an undeclared `s_uart_ctx`. The temporary harness was not
  executed on the embedded target.
- Hardware: Not performed. This pilot preserves existing GPIO/timer behavior;
  hardware smoke testing remains pending.

## 2026-09-12 - SPI1/SPI2 CubeMX-independent port migration

- Agent: Codex
- Commit: working tree
- Summary: Continued the SPI framework migration after the user confirmed
  SPI2 pin mapping (`PB13=SCK`, `PB14=MISO`, `PB15=MOSI`) and requested that
  `.ioc` be left out of the work. SPI is currently polling/blocking only;
  SPI DMA remains disabled. Port-owned SPI contexts now own the handles,
  clocks, GPIO AF setup, and default SPI parameters. `HAL_SPI_Init()` invokes
  the Core MSP dispatcher, which forwards owned handles to the port MSP
  implementation; the public SPI ops and transaction API remain lightweight
  and HAL-free above `Usr/port`. The SPI ops now also expose a symmetric
  `deinit` operation: `dev_spi_deinit()` rejects active transactions, the port
  backend calls `HAL_SPI_DeInit()`, and it clears `initialized` only after a
  successful deinitialization. Port ownership checks require the address of
  one of the private SPI handles rather than only matching `Instance`.
- Files: `Usr/port/port_bsp/port_hal_spi.c`,
  `Usr/port/port_bsp/port_hal_spi.h`, `Usr/port/board_support.c`,
  `Usr/port/usr_port_platform.c`, `Usr/drivers/bus/dev/dev_spi.c`,
  `Usr/drivers/bus/dev/dev_spi.h`, `Core/Src/spi.c`, `Core/Inc/spi.h`,
  `Core/Src/stm32wlxx_it.c`, `Core/Inc/dma.h`, `Core/Src/dma.c`,
  `Core/Src/adc.c`, `Usr/DEVICE_DRIVER_FRAMEWORK.md`,
  `docs/change_log/CHANGELOG.md`, `docs/ai-generation-log.md`,
  `tmp/spi_msp_structure_test.py`.
- Verification: Fresh clangd diagnostics reported 0 errors and 0 warnings
  for `port_hal_spi.c`, `port_hal_spi.h`, `Core/Src/spi.c`, `Core/Inc/spi.h`,
  `dev_spi.c`, `dev_spi.h`, `dri_ops.h`, and `board_support.c`; `usr_app.c`
  reported 0 errors and 2 existing unused-variable warnings. Clangd symbol
  references and call hierarchy resolved the current chain from
  `HAL_SPI_MspInit()` to `usr_port_hal_spi_msp_init()` and from the port init
  operation to `HAL_SPI_Init()`, with no stale generated-handle result.
  Source scans confirmed no HAL/vendor matches in `Usr/drivers` or `Usr/device`,
  no generated SPI init/handle/DMA names under `Core` and `Usr`, and a single
  port-side SPI MSP resource owner in `Usr/port/port_bsp/port_hal_spi.c`. The
  convenience transaction error path was also corrected so a failed
  `dev_spi_begin()` cannot call `dev_spi_end()` for a transaction owned by
  another caller. `python .\\tmp\\spi_msp_structure_test.py` passed,
  including the regression check that MSP ownership requires a private handle
  address. The fresh `git diff --check` exited 0. The fresh
  `python .\\scripts\\build.py` exited 0, linked `KG200Z_OPEN.elf`, and
  reported `[BUILD] SUCCESS (0 errors, 0 warnings)`; no SPI-specific build or
  link error was reported.
- Hardware: Not performed. Pending FRAM RDID/boundary read-write, ICP probe
  and continuous sampling, SPI1/SPI2 waveforms, CS timing, ADC interaction,
  USART2/LPUART1 trace, GPS USART1 DMA, and DMA IRQ observation.
- Scope note: `.ioc` was neither modified nor treated as an acceptance item.

## 2026-09-11 - UART MSP hardcoding elimination final verification

- Agent: Codex
- Commit: working tree
- Summary: Ran Task 6 final verification for the cumulative UART MSP clock and
  DMA ownership decoupling work, then recorded the evidence and remaining
  hardware risks without changing firmware source, headers, or build scripts.
- Files: `docs/change_log/CHANGELOG.md`, `docs/ai-generation-log.md`,
  `.superpowers/sdd/2026-09-10-uart-clock-dma-resource-decoupling/task-6-report.md`
- Verification: fresh `rg` and current disk source show no `hdma_usart1_rx`, show `s_usart1_rx_dma`, `usr_port_hal_uart_owns`, and `usr_port_hal_uart_dma_irq`, and show the Channel2 IRQ ownership mapping. Clangd MCP `get_diagnostics(force_refresh=true)` reported 0 errors/0 warnings for `port_hal_uart.c`, `usr_port_hal.h`, `usart.c`, `usart.h`, and `stm32wlxx_it.c`; `board_support.c` reported 0 errors plus 3 existing unused-variable warnings. The immediately following `workspace_symbol_search`, `get_document_symbols`, `find_references`, and `get_call_hierarchy` calls read a stale background snapshot: they still reported `Core/Src/usart.c:31` `hdma_usart1_rx`, missed the new symbols, showed old `port_hal_uart.c` line 406/455 and old Core DMA document symbols, and could not resolve the new references/hierarchy. Therefore symbol/reference/document/hierarchy results are not ownership or call-chain evidence; fresh `rg`, disk source, and successful build are authoritative. Full-worktree `git diff --check; $code=$LASTEXITCODE; "EXIT_CODE=$code"` exited 2 only because user-modified `CMakeLists.txt:261` has trailing whitespace. Scoped UART/doc diff-check exited 0 with only LF/CRLF warnings. An earlier full-link build exited 0 with `[BUILD] SUCCESS (0 errors, 22 warnings)` and non-empty ELF/HEX; the latest incremental build exited 0 with `[100%] Built target KG200Z_OPEN` and `[BUILD] SUCCESS (0 errors, 0 warnings)`.
- Follow-up: Board validation remains pending for GPS USART1 RX DMA, GPS reset
  and deinit/reinit parsing, non-owned UART MSP bypass, DMA1_Channel2 IRQ
  dispatch count, ADC Channel1 battery path, USART2/LPUART1 trace output, and
  SPI1 non-DMA behavior. Do not infer ownership from clangd's stale background
  index, and do not claim the build output contained `Finished building`.

<!-- END_SOURCE_FRAGMENT S018-F4 -->
