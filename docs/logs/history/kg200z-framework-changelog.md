# Imported framework change records

> Authority: historical source excerpt, not current API or current verification.
> Source: S017, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/logs/CHANGELOG.md`.

Selected framework change entries. Some host-specific context remains necessary to understand ownership evolution; application persistence and chip-driver entries excluded.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 10-138

<!-- SOURCE_FRAGMENT S017-F1 -->
## 2026-10-09 - Multi-channel timer PWM output

- Extended the compact PWM port to support up to four statically configured
  channels sharing timer clock, PSC, and ARR, with independent GPIO/AF,
  polarity, and initial CCR values.
- Changed `dev_pwm_set_compare()` to accept logical channel `1..4` and a raw
  compare count. Group start/stop and initialization rollback cover all rows.
- No physical PWM row was added because the current board mapping is not
  confirmed; no target flash, register, RTT, or waveform verification was run.

## 2026-10-09 - Compact static TIM/PWM port adapter

- Removed input-capture configuration, capture event storage, owner/claim
  arbitration, prescaler/ARR search, PWM quantization, and derived timer state.
- `port_hal_tim` now owns one static TIM resource model shared by the timer and
  PWM facades. Each resource selects BASIC or PWM, never both at once.
- Board resources provide `timer_clock_hz`, PSC, ARR, and PWM channel/compare
  values directly. TIM2 remains the scheduler resource at 48 MHz, PSC 0, ARR
  47999 for the existing 1 ms update tick.
- PWM now exposes raw compare-count updates. No confirmed PWM board row was
  added; `get_pwm_device()` remains unavailable on the current board.
- Host tests were migrated from the removed `Usr/port` layout to `Usr/hal`.
  Live PWM waveform, capture behavior, and target register observation remain
  unverified.

## 2026-10-02 - Semantic timer, capture, and PWM resource architecture

**Architecture**

- UART, SPI, timer, and PWM public configuration now uses application-facing
  values. STM32 HAL enums, clock selection, prescaler, ARR, CCR, GPIO AF, IRQ,
  and resource ownership remain in `Usr/port`.
- Timer resources are static board descriptors with explicit capabilities,
  owner roles, lifecycle state, claim masks, capture event storage, and
  scheduler reservation. TIM2 remains exclusively the scheduler's 1 ms basic
  counter and continues to use the existing RTC timebase path.
- The timer port now supports semantic basic timing and a reusable input
  capture flow with bounded ISR event production, foreground ring consumption,
  wrap/timeout handling, and observable dropped-event counts.
- The new PWM device is a static TIM-backed port adapter. Channels on one TIM
  share PSC/ARR, frequency updates prepare every active CCR and commit them as
  one transaction with rollback on HAL update failure.
- UART, TIM, and PWM adapters retain claims when rollback or cleanup fails and
  enter a fault state. Normal operations are rejected until an explicit
  `deinit` retry completes the HAL cleanup and releases the resource.
- SPI transfer lengths remain byte counts at the reusable API. In 16-bit mode,
  the port validates even length and alignment before converting bytes to the
  frame count required by STM32 HAL.
- The current board has no confirmed PWM or capture pin/resource row, so those
  tables are empty and normal boot does not create or start either resource.

**Compatibility / Scope**

- `NULL` initialization configuration still selects the board default for
  UART, timer, and PWM resources.
- GPS remains bound to USART1, the trace path remains on USART2/RTT, and the
  existing application scheduler remains on TIM2.
- No unverified physical PWM/capture pin was added to the board binding or
  generated CubeMX files. `get_pwm_device()` returns `NULL` on this board and
  callers must treat that as unsupported.
- `conflict_domain` is currently board-resource metadata. With only one live
  TIM2 descriptor, no cross-descriptor conflict arbiter is implemented or
  claimed as verified.

**Verification**

- Host semantic and lifecycle tests passed: resource ownership `25` checks,
  timer calculator/capture ring/claim tests, UART semantic tests, UART/SPI
  lifecycle rollback tests, and TIM/PWM lifecycle rollback tests.
- A fresh `python .\\scripts\\build.py` passed with `0 errors, 119 warnings`
  and generated non-empty ELF/HEX/BIN artifacts. The resulting size was
  `text=258364`, `data=2004`, `bss=18044`; Flash load is `260368` bytes and
  remaining Flash is `1776` bytes. The warning set contains repeated existing
  RTC macro redefinitions, existing unused/return/type/format diagnostics, and
  the existing linker RWX LOAD segment warning.
- Hardware programming and runtime evidence are recorded separately in the
  completion record and must not be inferred from the build result.

**Risks / Follow-up**

- Physical PWM output, input capture, and BOTH-edge behavior remain unverified
  because this board has no confirmed resource row. BOTH-edge classification
  currently samples GPIO IDR in the ISR and may be affected by ISR timing or
  fast signal transitions.

## 2026-09-24 - UART/SPI/I2C single-instance board resources

**Architecture**

- Each physical UART, SPI, and I2C peripheral now has one strongly typed
  mutable instance containing its generic device, HAL handle or handle
  reference, and runtime state.
- `board_support.c` owns the const GPIO, clock, DMA, IRQ, and default-init
  resource tables. The `port_hal_*` modules only implement HAL behavior over
  an injected instance.
- UART RX/context/device parallel arrays, the UART bind registry, and the
  SPI/I2C private context tables and runtime `get_context()` wiring were
  removed.
- Core MSP dispatch calls one boolean `*_msp_init/deinit()` entry instead of
  performing a separate `owns()` lookup.
- `uart_device_config_t` no longer exposes an MCU instance pointer. The HAL
  instance is selected only from the board resource.

**Compatibility**

- `get_uart_device()`, `get_spi_device()`, `get_spi0_device()`,
  `get_i2c_device()`, and all existing `dev_*` APIs keep their previous
  application-facing behavior.
- USART1 remains circular-DMA RX, USART2 remains one-byte interrupt RX, and
  LPUART1 RX/IRQ remains disabled.
- SPI1, SPI2, and I2C1 remain synchronous buses; their optional DMA resources
  remain disabled.

**Verification**

- The diagnostic Host Ring test passed software wrap/drop-oldest, direct DMA
  commits, DMA overwrite accounting, capacity 242 position wrap, and epoch
  wrap across `UINT32_MAX`.
- `python .\scripts\peripheral_resource_structure_test.py all` passed all
  12 ownership checks, and `git diff --check` passed with only local Git
  permission and line-ending notices.
- The final build reported `[BUILD] SUCCESS (0 errors, 0 warnings)` and
  generated non-empty ELF, HEX, and BIN artifacts.
- ELF size changed from `data=1664, bss=18056` to
  `data=1664, bss=18048`, reducing static RAM by 8 bytes. Symbol inspection
  showed only the new resource and instance tables; the old parallel context
  and device symbols were absent.
- The project flash script reported `Verified OK`, target reset, and
  `[FLASH] SUCCESS`.
<!-- END_SOURCE_FRAGMENT S017-F1 -->

## Source lines 141-145

<!-- SOURCE_FRAGMENT S017-F2 -->
- DMA1 Channel2 was enabled with a changing transfer count, USART1 RDR as its
  peripheral address, and the new UART ring storage as its memory address.
  Ring positions/epochs advanced while the foreground consumer kept up.
- USART2 had RX interrupt enabled and DMAR disabled. DMA Channel3 and LPUART1
  registers remained disabled; NVIC enabled USART1/USART2 but not LPUART1.
<!-- END_SOURCE_FRAGMENT S017-F2 -->

## Source lines 149-373

<!-- SOURCE_FRAGMENT S017-F3 -->
- `SCB.CFSR` and `SCB.HFSR` were both zero after the runtime checks.

## 2026-09-24 - Direct DMA ownership of the unified ring buffer

**Purpose**

- Remove the USART1 DMA staging buffer and make the unified ring storage the
  direct target of circular DMA.
- Replace monotonic producer/consumer counters with read/write positions plus
  epochs so ring state is explicit for software and DMA producers.
- Make UART NVIC enablement an explicit board binding: USART1 DMA keeps UART
  error IRQ enabled, USART2 IT keeps UART IRQ enabled, and LPUART1 NONE
  disables its UART IRQ.

**Behavioral Contract**

- Software/IT mode calls `ring_buf_write()` and copies bytes into the ring.
- DMA mode calls `ring_buf_dma_commit()` after HT/TC and does not copy bytes;
  DMA has already written `ring->buffer`.
- Circular DMA uses a safe unread window of `capacity / 2`. If the DMA
  producer crosses a half-buffer boundary before the consumer has read the
  previous half, the oldest unread data is counted as overflow.
- DMA callback lateness is tracked by `dma_missed_cycles`; overwritten bytes
  cannot be reconstructed.

**Verification**

- ARM ring syntax checks cover software wrap, drop-oldest, DMA commit without
  copying, DMA overwrite overflow, epoch wrap, and invalid arguments.
- Structural checks cover no staging buffer, direct DMA-to-ring ownership,
  DMA commit usage, position/epoch fields, and board-level UART IRQ settings.
- The project build passed with 0 errors and 2 existing warnings.

**Risks**

- The direct DMA design intentionally trades full physical storage for a
  half-buffer safe unread window. Sustained GPS traffic still requires live
  observation of overflow and missed-cycle counters.
- Final direct-DMA flash completed with `Verified OK` and target reset. After
  reset, the external USB-UART device re-enumerated from [historical serial port] to [historical serial port]/[historical serial port];
  neither new port returned the CLI probe command, so direct-DMA runtime CLI
  validation is pending and no prior [historical serial port] result is attributed to this image.

**Final Dynamic Verification**

- [historical serial port] on the reconnected final image returned `ver` with `MCU: STM32WLE5JC`,
  `HAL: 1.3.0`, `SysClk: 48000000 Hz`, and `param show` with
  `log_level=1 dirty=0`.
- MKLink [historical serial port] connected with target IDCODE `0x6BA02477`.
- Final ELF symbol mapping: ring buffer storage `0x20002F2C`, RX contexts at
  `0x200003E4`, and USART1 DMA handle at `0x20003180`.
- DMA1 Channel2 raw RAM: `CCR=0xAF` (enabled), `CNDTR` non-zero and changing,
  `CPAR=0x40013824` (USART1 RDR), `CMAR=0x20002F2C` (unified ring buffer).
- USART1 `CR3=0x41`, confirming DMAR and UART error interrupt enablement.
- RX contexts showed USART1 mode DMA with UART IRQ enabled and RX enabled;
  USART2 mode IT with UART IRQ enabled and RX enabled; LPUART1 context zeroed
  for NONE mode.
- Two ring snapshots showed the DMA write epoch/position advancing. The
  overflow field was readable and non-zero during the observation window,
  consistent with the documented half-buffer safe-window/drop-oldest model.
- Raw CFSR/HFSR memory read at `0xE000ED28` returned eight zero bytes.

## 2026-09-24 - Unified software ring for UART DMA and interrupt RX

**Purpose**

- Replace the separate software and DMA ring implementations with one
  position/epoch ring that supports both software/interrupt production and
  direct circular-DMA production.
- Remove the UART foreground `NDTR` refresh path, which could miss a complete
  DMA wrap between polls and could read memory while DMA was overwriting it.
- Make the board RX modes explicit: USART1/GPS uses circular DMA, USART2/CLI
  uses one-byte interrupt reception, and LPUART1 RX remains disabled.

**Files Changed**

- `Usr/component/ring_buf.[ch]` - Add the single `ring_buf_t` API with
  read/write positions, epochs, DMA half-buffer commit, drop-oldest overflow,
  and ARM memory barriers; remove `Ring_Buf_t`/`Ring_dma_t` and old APIs.
- `Usr/port/port_bsp/port_hal_uart.[ch]` and `Usr/port/board_support.c` - Add
  RX mode contexts, direct USART1 DMA-to-ring ownership, HT/TC commit,
  USART2 IT rearming, explicit UART IRQ configuration, mode-gated MSP/DMA
  ownership, and unified reads.
- `Core/Src/usart_if.c` - Dispatch UART half-complete, complete, and error
  callbacks to the port owner while preserving the LPUART legacy callback.
- `Usr/drivers/dev/dev_lorawan.c` - Migrate LoRaWAN receive buffering to the
  same ring write/read API.
- `scripts/peripheral_resource_structure_test.py` - Add 2 ring/UART mode
  ownership checks, for 11 total structural checks.

**API / Protocol Impact**

- The old `Ring_Buf_t`, `Ring_dma_t`, `save_ring_buf`, `read_ring_buf`,
  `refresh_ring_buf_dma`, `get_ring_dma_available`, and `read_ring_dma` APIs
  are removed from project-owned code.
- `ring_buf_read()` returns the actual number of bytes read instead of an
  all-or-nothing status. Overflow retains the newest bytes and counts dropped
  oldest bytes.
- No CLI, GPS, LoRaWAN, or wire protocol format changed.

**Verification**

- `python .\\scripts\\peripheral_resource_structure_test.py all`: passed
  11 checks.
- `git diff --check`: exit code 0, with existing LF/CRLF and Git ignore
  permission warnings only.
- `python .\\scripts\\build.py`: `[BUILD] SUCCESS (0 errors, 4 warnings)`;
  non-empty ELF/HEX/BIN generated. Warnings are existing RTC macro
  redefinitions and the existing RWX linker warning.
- `arm-none-eabi-gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only` passed for
  the host ring test and ring implementation.
- Follow-up review fixes publish UART RX state before enabling HAL reception,
  preserve already committed ring data during deferred error recovery, return
  unsupported for NONE-mode reads, isolate the legacy LPUART callback, and
  count DMA half-buffer callbacks that arrive after the DMA has crossed the
  safe staging boundary.

**Risks / Follow-up**

- DMA half-buffer callbacks must run before the DMA reuses that half; this is
  a runtime scheduling constraint. `dma_missed_cycles` records callbacks that
  arrive after the safe staging boundary, but bytes already overwritten by
  DMA cannot be recovered; board observation under sustained GPS traffic is
  still required.
- The host GCC processes on this machine did not return within 30 seconds, so
  the host executable ring tests were not run; ARM syntax and full firmware
  link passed instead.
- Before the final hardware pass, live USART1 DMA counters and fault-register
  reads remained pending; the final hardware evidence is recorded below.

**Final Hardware Verification**

- Final rebuild after review fixes: `python .\\scripts\\build.py` reported
  `[BUILD] SUCCESS (0 errors, 0 warnings)`.
- Final HEX flash: `python .\\scripts\\flash.py` reported `Verified OK`,
  `Resetting Target`, and `[FLASH] SUCCESS - Verified OK, Target Reset`.
- [historical serial port] CLI on the final image returned `ver` with `MCU: STM32WLE5JC`,
  `HAL: 1.3.0`, and `SysClk: 48000000 Hz`; `param show` returned
  `log_level=1 dirty=0`. This confirms the USART2 interrupt RX path reaches
  the CLI after the final reset.
- MKLink 0.1.6 target RAM/register reads returned a fixed invalid-looking
  pattern for both peripheral and SCB addresses. An OpenOCD read-only session
  halted the target but did not emit the requested memory words and affected
  run state, so no ring-counter, DMA-register, or CFSR/HFSR numeric claim is
  made. The final firmware was re-flashed and reset afterward.

## 2026-09-22 - Shared DMA resources and macro-generated peripheral clocks

**Purpose**

- Consolidate UART, SPI, and I2C port DMA configuration into a nested resource
  descriptor and remove DMA initialization hardcoding from UART MSP code.
- Generate the four peripheral clock callbacks from port-local macros while
  keeping the HAL-specific clock selection field explicit per peripheral.

**Files Changed**

- `Usr/port/port_bsp/port_hal_dma.[ch]` - Add the private DMA descriptor and
  shared init, IRQ, and deinit lifecycle helpers.
- `Usr/port/port_bsp/port_hal_uart.c` - Move USART1/USART2/LPUART1 RX DMA
  configuration into descriptors and keep Channel2/3/6 mappings.
- `Usr/port/port_bsp/port_hal_spi.c` and `port_hal_i2c.c` - Add nested,
  disabled DMA descriptors and conditional MSP support without enabling DMA for
  the current polling resources.
- `Usr/port/port_bsp/port_hal_clock.h` - Add macro-generated clock callbacks.
- `Core/Src/adc.c`, `Core/Src/adc_if.c`, `Core/Src/stm32wlxx_it.c`,
  `Core/Src/usart_if.c`, and `Usr/port/usr_port_platform.c` - Move DMA clock
  ownership to active consumers and remove the global `MX_DMA_Init` path.
- `scripts/peripheral_resource_structure_test.py` - Add resource, lifecycle,
  macro, and IRQ ownership checks.

**API / Protocol Impact**

- No public `Usr/drivers` or `Usr/device` API changed. SPI and I2C remain
  blocking/polling interfaces.
- UART RX DMA remains circular byte mode on DMA1 Channels 2, 3, and 6; ADC
  remains on DMA1 Channel1.

**Layering / Dependency Impact**

- HAL and concrete DMA types remain confined to `Usr/port` and the existing
  CubeMX ADC/IRQ service files.
- DMA controller clocks are enabled by the consumer before `HAL_DMA_Init` and
  are not disabled by an individual consumer deinit.

**Verification**

- `python .\\scripts\\peripheral_resource_structure_test.py all`: passed 9
  resource ownership checks.
- `python .\\scripts\\build.py`: passed with 0 errors and 22 existing
  warnings; ELF, HEX, and BIN were generated.
- `python .\\scripts\\flash.py`: passed with `Verified OK`, `Resetting
  Target`, and `[FLASH] SUCCESS` on the connected STM32WLE/WL5x target.
- `git diff --check`: passed with only existing LF/CRLF and Git ignore
  permission warnings.

**Risks / Follow-up**

- Hardware programming and reset validation passed. Runtime peripheral smoke
  observations for GPS RX DMA, ADC DMA, trace, SPI/I2C polling, and
  deinit/reinit IRQ behavior remain pending.

**Dynamic Hardware Validation**

- External CLI UART responded to `help`, `ver`, `param show`, and `loglevel`.
  The version response reported STM32WLE5JC, HAL 1.3.0, and a 48 MHz system
  clock.
- Runtime parameter validation passed: `param set log_level 2` marked the
  parameter dirty, `param save` cleared dirty state, and the value was then
  restored to the original level 1 and saved.
- MKLink SWD connectivity and target IDCODE read passed. `SCB.CFSR` and
  `SCB.HFSR` both read zero during the observation window.
- DMA controller RAM inspection confirmed ADC Channel1 disabled in the current
  application path, USART1 RX Channel2 enabled with USART1 RDR and the GPS
  ring buffer addresses, and USART2 RX Channel3 enabled with USART2 RDR and
  the CLI ring buffer addresses. LPUART1 Channel6 remained disabled because
  its application receive path is not started.
- GPS circular DMA activity was observed: the ring produced/consumed counters
  and write index advanced between two snapshots. SPI1/SPI2 and I2C1 contexts
  were initialized with DMA links remaining null, matching the polling-only
  scope of this change.
- RTT dynamic startup succeeded at the configured control block address, but
  no RTT payload was captured during the sample window. The external trace
  UART also produced no bytes during its capture window.

<!-- END_SOURCE_FRAGMENT S017-F3 -->

## Source lines 521-666

<!-- SOURCE_FRAGMENT S017-F4 -->
## 2026-09-14 - UART DMA IRQ resource-table ownership

**Purpose**

- Remove UART DMA interrupt routing that depended on specific UART handles or
  a hardcoded DMA Channel 3 conflict check.
- Keep NVIC setup and teardown with the MSP resource owner so UART resources
  can be added through the table without changing generic IRQ dispatch code.

**Files Changed**

- `Usr/port/port_bsp/port_hal_uart.c` - Add UART and RX-DMA IRQ metadata to
  each resource, configure and disable those IRQs in UART MSP init/deinit, and
  dispatch DMA IRQs by `IRQn_Type` through the resource table.
- `Usr/port/usr_port_hal.h` - Change the DMA IRQ adapter API to accept an IRQ
  number and expose the UART IRQ adapter used by the Core handlers.
- `Core/Src/stm32wlxx_it.c` and `Core/Inc/stm32wlxx_it.h` - Route USART and
  DMA IRQs to the port adapters and add the LPUART1 RX DMA Channel 6 handler.
- `Core/Src/usart_if.c` - Remove legacy trace-layer DMA NVIC teardown; the
  active trace path is RTT and UART MSP owns UART DMA lifecycle.
- `tmp/msp_nvic_ownership_test.py` - Add static checks for unique DMA channel
  ownership, Channel 6 routing, MSP NVIC lifecycle, and table-driven dispatch.

**API / Protocol Impact**

- `usr_port_hal_uart_dma_irq()` now accepts `IRQn_Type irq` instead of a UART
  handle. Callers must pass the IRQ currently being serviced.
- USART1 RX uses DMA1 Channel 2, USART2 RX uses DMA1 Channel 3, and LPUART1
  RX uses DMA1 Channel 6. ADC retains DMA1 Channel 1.
- LPUART1 board binding uses `PB10/PB11` with `GPIO_AF8_LPUART1`.

**Layering / Dependency Impact**

- Concrete UART instances, DMA channels, IRQ numbers, priorities, clocks,
  pins, and HAL handles remain in `Usr/port/port_bsp/port_hal_uart.c`.
- UART MSP owns per-resource NVIC enable, priority, and disable operations.
  The Core interrupt file only forwards the active IRQ to the adapter.
- DMA controller clock setup remains in `MX_DMA_Init()`; SPI and I2C remain
  polling paths with no newly enabled IRQ or DMA ownership.

**Verification**

- `python .\\tmp\\msp_nvic_ownership_test.py`: exit code 0; `PASS: 12
  MSP/NVIC ownership checks`.
- `rg -n "usr_port_hal_uart_dma_irq|usr_port_hal_uart_dma_channel3_irq|DMA1_Channel6|s_usart2_rx_dma|s_lpuart1_rx_dma|resource->rx_dma_instance == DMA1_Channel3" Core Usr tmp`: confirmed the new IRQ API and Channel 6 mappings; no old Channel 3 conflict predicate or old IRQ API was found.
- `git diff --check -- Core/Inc/stm32wlxx_it.h Core/Src/stm32wlxx_it.c
  Core/Src/usart_if.c Usr/port/port_bsp/port_hal_uart.c
  Usr/port/usr_port_hal.h tmp/msp_nvic_ownership_test.py`: exit code 0;
  only Git line-ending conversion warnings were emitted.
- `python .\\scripts\\build.py`: exit code 0; `[100%] Built target
  KG200Z_OPEN` and `[BUILD] SUCCESS (0 errors, 16 warnings)`. Warnings are
  existing RTC macro redefinitions and the linker RWX-segment warning.
- `Get-Item .\\cmake-build\\KG200Z_OPEN.elf`: verified after the build as a
  generated target artifact; no hardware test was run.

**Risks / Follow-up**

- Board validation is still required for USART1 GPS RX DMA Channel 2,
  USART2 RX Channel 3, LPUART1 RX Channel 6, ADC Channel 1, IRQ callbacks,
  and the actual LPUART1 PB10/PB11 wiring.
- DMA Channel 4/5 handler remnants remain for inactive legacy TX-DMA code;
  they are not enabled by the current trace path and were left unchanged.

## 2026-09-12 - Guard macro pilot for GPIO and timer wrappers

**Purpose**

- Reduce repeated defensive `if`/`return` blocks while keeping failure
  predicates and return behavior explicit at each call site.
- Validate the approach on low-risk GPIO and timer wrapper guards before any
  broader driver-framework migration.

**Files Changed**

- `Usr/common/usr_check.h` - Add statement-safe return guard macros, including
  opt-in logging variants.
- `Usr/drivers/bus/dev/dev_gpio.c` - Migrate the `write`, `read`, and `toggle`
  parameter and ops-table guards to `USR_RETURN_IF`; keep the IRQ/callback
  registration guard explicit.
- `Usr/drivers/bus/dev/dev_timer.c` - Migrate the `init`, `start`, and `stop`
  guards to `USR_RETURN_IF`; keep the `get_ticks` value-return guard explicit.

**API / Protocol Impact**

- Add `USR_RETURN_IF`, `USR_RETURN_VOID_IF`,
  `USR_RETURN_STATUS_IF_NOT_OK`, `USR_RETURN_IF_LOG`, and
  `USR_RETURN_VOID_IF_LOG` as internal framework helper macros.
- A true macro condition means failure and immediate return. GPIO and timer
  public APIs, operation signatures, and returned status values are unchanged.

**Layering / Dependency Impact**

- The guard header depends only on the existing `usr_common.h` contract; it
  adds no HAL, LL, or vendor dependency below `Usr/port`.
- The pilot uses only silent macros. Logging guards remain explicit opt-ins
  because the current log path can call back into the UART framework.
- SPI transactions, UART, DMA, MSP, IRQ/callback, cleanup, rollback, lock, and
  state-transition paths remain explicit and unchanged by this pilot.

**Verification**

- `arm-none-eabi-gcc -std=c11 -Wall -Wextra -Werror -ffreestanding
  -fsyntax-only -I .\Usr\common -I .\Usr\log
  -I .\Usr\drivers\bus\dev .\tmp\usr_check_macro_test.c
  .\Usr\drivers\bus\dev\dev_gpio.c
  .\Usr\drivers\bus\dev\dev_timer.c`: exit code 0. The temporary harness
  covers all five macros, status propagation, single condition evaluation,
  `if/else` statement safety, logging enabled/disabled behavior, and the real
  GPIO/timer wrappers with fake operations.
- `[local tool path] ...
  .\tmp\usr_check_macro_test.c .\Usr\drivers\bus\dev\dev_gpio.c
  .\Usr\drivers\bus\dev\dev_timer.c
  -o .\tmp\usr_check_macro_test.exe`, followed by
  `.\tmp\usr_check_macro_test.exe`: compile exit code 0 and run exit code 0.
  The harness was not executed on the embedded target.
- Fresh clangd diagnostics for `usr_check.h`, `dev_gpio.c`, and `dev_timer.c`:
  0 errors and 0 warnings in each file.
- `arm-none-eabi-gcc -std=c11 -Wall -Wextra -Werror -ffreestanding
  -fsyntax-only -I .\Usr\common -I .\Usr\log
  -I .\Usr\drivers\bus\dev .\Usr\drivers\bus\dev\dev_gpio.c
  .\Usr\drivers\bus\dev\dev_timer.c`: exit code 0 after the temporary
  harness was removed.
- `git diff --check -- Usr/drivers/bus/dev/dev_gpio.c
  Usr/drivers/bus/dev/dev_timer.c docs/change_log/CHANGELOG.md
  docs/ai-generation-log.md`: exit code 0 for the tracked pilot changes.
- `rg -n "[ \t]+$" Usr/common/usr_check.h
  docs/superpowers/plans/2026-09-12-usr-check-macro-pilot.md`: no matches
  (expected exit code 1) in the two untracked pilot files.
- Full `git diff --check`: exit code 2, blocked outside this pilot by trailing
  whitespace in the concurrently modified `Usr/port/port_bsp/port_hal_uart.h:21`.
- Final `python .\scripts\build.py`: exit code 1. The guard pilot sources
  compiled, but the build stopped in the concurrently modified UART port:
  `usr_port_hal_uart_resource_t` is defined incompatibly in both
  `port_hal_uart.h` and `port_hal_uart.c`, and `port_hal_uart.c` references an
  undeclared `s_uart_ctx`. These UART files were not changed by this pilot.

**Risks / Follow-up**

- No board behavior was changed, but hardware smoke testing was not performed.
- The full firmware build remains blocked by the separate in-progress UART
  edits described above; resolve those edits and rerun the full build before
  integration.
- Broader macro adoption requires separate review by path category. Do not
  mechanically convert resource cleanup, ownership, ISR/callback, transaction,
  or state-transition control flow.

<!-- END_SOURCE_FRAGMENT S017-F4 -->

## Source lines 667-759

<!-- SOURCE_FRAGMENT S017-F5 -->
## 2026-09-12 - SPI1/SPI2 port-owned polling driver migration

**Purpose**

- Remove the SPI1/SPI2 driver path's dependency on CubeMX-generated SPI
  handles, `MX_SPI1_Init()`, `MX_SPI2_Init()`, and Core SPI resource ownership.
- Keep the reusable SPI layer independent of STM32 HAL while retaining a
  static `ops + context` backend and logical GPIO chip-select control.

**Files Changed**

- `Usr/port/port_bsp/port_hal_spi.c` and
  `Usr/port/port_bsp/port_hal_spi.h` - Own SPI1/SPI2 handles, resource
  records, clocks, GPIO alternate functions, default parameters, the common
  HAL-backed ops implementation, and the port-side MSP implementation.
- `Usr/port/board_support.c` and `Usr/port/usr_port_platform.c` - Bind opaque
  SPI contexts and stop calling the Core `MX_SPI*_Init()` functions.
- `Usr/drivers/bus/dev/dev_spi.c` and `Usr/drivers/bus/dev/dev_spi.h` - Add
  `dev_spi_init()` while preserving transaction and multi-frame APIs. The
  convenience transaction now returns immediately when `dev_spi_begin()`
  cannot acquire the bus, so it cannot release another owner's chip select.
- `Core/Src/spi.c`, `Core/Inc/spi.h`, `Core/Src/stm32wlxx_it.c`,
  `Core/Inc/dma.h`, `Core/Src/dma.c`, `Core/Src/adc.c`, and related includes -
  Remove generated SPI handles, SPI DMA remnants, SPI IRQ routing, and the
  obsolete PB13 ADC configuration. `Core/Src/spi.c` now only dispatches the
  HAL MSP callbacks to the port adapter.
- `Usr/DEVICE_DRIVER_FRAMEWORK.md` - Document current SPI ownership, ops,
  pins, chip-select resources, polling mode, and future DMA boundary.

**API / Protocol Impact**

- `dev_spi_ops_t` now provides symmetric `init(ctx)`, `deinit(ctx)`, and
  `transmit(ctx, tx, tx_len, rx, rx_len, timeout_ms)` operations.
- `dev_spi_deinit()` rejects an active SPI transaction and delegates to the
  port backend. The port backend maps `HAL_SPI_DeInit()` to project status
  codes and clears its initialized state only after successful deinit. The
  MSP ownership probe accepts only the private handles returned through the
  port context binding, preventing an unrelated handle with the same SPI
  instance from releasing these resources.
- `dev_spi_init()` now follows the HAL MSP lifecycle: the port adapter calls
  `HAL_SPI_Init()`, HAL calls Core's `HAL_SPI_MspInit()`, and Core dispatches
  owned handles to `usr_port_hal_spi_msp_init()`. Deinitialization follows the
  corresponding `HAL_SPI_MspDeInit()` path.
- `dev_spi_init()` is now called for both SPI devices during application
  initialization. Existing `dev_spi_transact()`, `dev_spi_begin()`,
  `dev_spi_frame()`, and `dev_spi_end()` APIs remain in use.
- SPI1 defaults to `PA5=SCK`, `PA6=MISO`, `PA7=MOSI`; SPI2 defaults to
  `PB13=SCK`, `PB14=MISO`, `PB15=MOSI`. Both use AF5, master 2-line full
  duplex, 8-bit, MODE0, MSB-first, software NSS, and prescaler 16.
- CS is a logical GPIO resource rather than a controller resource: SPI1 FRAM
  uses `PA4` and SPI2 ICP uses `PB12`.

**Layering / Dependency Impact**

- HAL handles, clock gates, GPIO AF setup, and physical SPI resources remain
  owned by `Usr/port/port_bsp/port_hal_spi.c`; `Core/Src/spi.c` retains only the
  HAL-required MSP dispatcher and does not own SPI resources.
- `Usr/drivers` and `Usr/device` retain no STM32 HAL dependency. No public
  `dma_device_t` was added.
- SPI DMA is not enabled in this phase. Future DMA handles, channels,
  requests, IRQ dispatch, transfer state, and buffer lifetime remain private
  to `Usr/port`.
- `.ioc` was intentionally not modified or used as an acceptance criterion.

**Verification**

- `mcp__clangd__get_diagnostics` with `force_refresh=true` on
  `port_hal_spi.c`, `board_support.c`, and `dev_spi.c`: 0 errors and 0
  warnings; `usr_app.c`: 0 errors and 2 existing unused-variable warnings.
- `python .\\tmp\\spi_msp_structure_test.py`: passed, including the private
  handle address ownership regression check.
- PowerShell source scans under `Usr/drivers` and `Usr/device` found no
  STM32 HAL/vendor types, HAL calls, or concrete peripheral types. The legacy
  SPI scan under `Core` and `Usr` found no `MX_SPI1_Init`, `MX_SPI2_Init`,
  `hspi1`, `hspi2`, `hdma_spi`, or `MX_DMA_SPI_Init` symbols.
- `git diff --check -- Core/Inc/spi.h Core/Src/spi.c
  Usr/port/port_bsp/port_hal_spi.c Usr/port/port_bsp/port_hal_spi.h
  Usr/DEVICE_DRIVER_FRAMEWORK.md docs/change_log/CHANGELOG.md
  docs/ai-generation-log.md`: exit code 0.
- `python .\\scripts\\build.py`: exit code 0; `[100%] Built target
  KG200Z_OPEN`, generated ELF/HEX/BIN, and the fresh build reported
  `[BUILD] SUCCESS (0 errors, 31 warnings)`. The warnings include RTC macro
  redefinitions, unrelated unused-variable/return-value and format-truncation
  warnings, and the linker RWX warning. No SPI-specific build error was
  introduced.

**Risks / Follow-up**

- Hardware validation is still pending for SPI1 FRAM RDID and boundary
  read/write, SPI2 ICP probe and continuous sampling, PB13/PB14/PB15 clock and
  data waveforms, PA4/PB12 CS timing, ADC interaction, trace paths, GPS
  USART1 RX DMA, and DMA IRQ behavior.

<!-- END_SOURCE_FRAGMENT S017-F5 -->

## Source lines 760-885

<!-- SOURCE_FRAGMENT S017-F6 -->
## 2026-09-11 - UART MSP clock callbacks and USART1 RX DMA ownership

**Purpose**

- Move USART1 kernel-clock selection, peripheral gate, and GPIO port gate out of
  the generic UART MSP dispatcher and into resource-specific callbacks.
- Restore `__HAL_LINKDMA()` for port-owned USART1 RX DMA and move that stream
  from DMA1_Channel1 to DMA1_Channel2 so ADC can keep Channel1.
- Make Core UART MSP port-first so adding another port-owned UART later is a
  resource record, not another Core `Instance==USARTx` branch.

**Files Changed**

- `Usr/port/port_bsp/port_hal_uart.c` - resource callbacks, static RX DMA
  handle, ownership probe, generic MSP dispatch, `__HAL_LINKDMA()`, DMA IRQ
  wrapper.
- `Usr/port/usr_port_hal.h` - `usr_port_hal_uart_owns()` and
  `usr_port_hal_uart_dma_irq()` declarations.
- `Core/Src/usart.c` - port-first USER CODE MSP, removed live USART1 dispatcher
  and generated `hdma_usart1_rx` definition.
- `Core/Inc/usart.h` - removed `hdma_usart1_rx` declaration.
- `Core/Src/stm32wlxx_it.c` - Channel1 ADC, Channel2 port USART1 RX.

**API / Protocol Impact**

- Public `dev_uart_*` API unchanged.
- New port-only APIs:
  - `bool usr_port_hal_uart_owns(const UART_HandleTypeDef *uartHandle);`
  - `void usr_port_hal_uart_dma_irq(UART_HandleTypeDef *uartHandle);`

**Layering / Dependency Impact**

- HAL/LL remain in `Usr/port` or preserved Core services.
- No public `dma_device_t` was added.
- USART2/LPUART1 remain Core-owned.

**Verification**

- `rg -n -C 5 "void usr_port_hal_uart_msp_(init|deinit)|void usr_port_hal_uart_dma_irq|bool usr_port_hal_uart_owns" Usr/port/port_bsp/port_hal_uart.c`: exit code 0; ownership probe, DMA IRQ wrapper, and generic MSP init/deinit found.
- `rg -n "RCC_PERIPHCLK_USART1|Usart1ClockSelection|RCC_USART1CLKSOURCE_PCLK2|__HAL_RCC_USART1_CLK_(ENABLE|DISABLE)|__HAL_RCC_GPIOA_CLK_ENABLE" Usr/port/port_bsp/port_hal_uart.c`: exit code 0; concrete clock/GPIO names appear at lines 35, 36, 42, 47, and 52 in resource-specific callbacks, not in generic MSP bodies.
- `rg -n "__HAL_LINKDMA\(|uartHandle->hdmarx|rx_dma_handle->Parent|HAL_DMA_Init\(|s_usart1_rx_dma|DMA1_Channel2|hdma_usart1_rx|usr_port_hal_uart_dma_irq|usr_port_hal_uart_owns|Instance==USART1|hdma_adc|hdma_spi1_rx" Usr/port/port_bsp/port_hal_uart.c Core/Src/usart.c Core/Inc/usart.h Core/Src/stm32wlxx_it.c Core/Src/spi.c Core/Src/adc.c`: exit code 0; `s_usart1_rx_dma` is static in `port_hal_uart.c`, Channel2 routes to `usr_port_hal_uart_dma_irq(&huart1)`, Channel1 routes to `hdma_adc`, live Core MSP uses `usr_port_hal_uart_owns()`, and `hdma_usart1_rx` was not found. `Instance==USART1` remains only in the inactive `#ifdef USART_ENABLE` legacy copy.
- `rg -n "stm32wlxx_hal|HAL_|UART_HandleTypeDef|DMA_HandleTypeDef|USART_TypeDef|GPIO_TypeDef" Usr/drivers Usr/device -g "*.[ch]"`: exit code 1; no HAL or concrete STM32 peripheral references found below `Usr/port`.
- clangd MCP `get_diagnostics(force_refresh=true)`: `port_hal_uart.c`, `usr_port_hal.h`, `usart.c`, `usart.h`, and `stm32wlxx_it.c` each reported 0 errors and 0 warnings; `board_support.c` reported 0 errors and 3 existing unused-variable warnings for `gps_device`, `icp_device`, and `hcd_device`. Diagnostics are current enough to show no new clangd errors in these files.
- Final clangd symbol-index re-verification: the immediately following `workspace_symbol_search`, `get_document_symbols`, `find_references`, and `get_call_hierarchy` calls read a stale background snapshot. It still reported `Core/Src/usart.c:31` `hdma_usart1_rx`, did not find `s_usart1_rx_dma`, `usr_port_hal_uart_owns`, or `usr_port_hal_uart_dma_irq`, showed old `port_hal_uart.c` line 406/455 document symbols and the old Core DMA variable, and could not resolve references or call hierarchy for the new symbols. These symbol/reference/document/hierarchy results are not ownership or call-chain evidence.
- Ownership and link conclusions use fresh `rg`, current disk source, and the successful supported build as authority: `hdma_usart1_rx` has no source match; the new ownership symbols and Channel2 IRQ mapping are present; firmware links successfully.
- `git diff --check; $code=$LASTEXITCODE; "EXIT_CODE=$code"`: exit code 2; full-working-tree formatting gate is blocked by pre-existing `CMakeLists.txt:261: trailing whitespace` in a user-protected file. Line-ending warnings were also reported for existing modified files.
- `git diff --check -- Core/Inc/usart.h Core/Src/usart.c Core/Src/stm32wlxx_it.c Usr/port/port_bsp/port_hal_uart.c Usr/port/usr_port_hal.h docs/change_log/CHANGELOG.md docs/ai-generation-log.md; $code=$LASTEXITCODE; "EXIT_CODE=$code"`: exit code 0; scoped UART implementation and Task 6 documentation files have no whitespace errors. Output contained only LF/CRLF warnings.
- Earlier full-link `powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1`: exit code 0; output included `[BUILD] SUCCESS (0 errors, 22 warnings)`, `[100%] Built target KG200Z_OPEN`, non-empty `cmake-build/KG200Z_OPEN.elf` at 2664900 bytes, and non-empty `cmake-build/KG200Z_OPEN.hex` at 661887 bytes.
- Latest incremental `powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1`: exit code 0; output included `[100%] Built target KG200Z_OPEN` and `[BUILD] SUCCESS (0 errors, 0 warnings)`. Neither build record claims the phrase `Finished building` appeared.

**Risks / Follow-up**

- Hardware validation is pending: GPS USART1 RX DMA, GPS reset/reinit parsing,
  non-owned UART MSP bypass, DMA1_Channel2 IRQ dispatch count, ADC Channel1
  battery reads, USART2/LPUART1 trace output, and SPI1 non-DMA operation still
  need board evidence.
- SPI DMA must not later reuse Channel1 or Channel2.
- Full-worktree `git diff --check` remains blocked by user-protected
  `CMakeLists.txt` trailing whitespace; scoped UART/doc diff-check passed, and
  this Task 6 pass did not modify that file.
- Clangd background symbol/reference/document/hierarchy indexing is stale and
  may fluctuate independently of current diagnostics. Continue using fresh
  `rg`, disk source, and a successful build for ownership and link conclusions.

## 2026-09-07 - UART port resource ownership split

**Purpose**

- Move GPS/USART1 board resource ownership out of the reusable UART bus object
  and into the STM32 port adapter, reducing dependency on CubeMX-generated
  `MX_USART1_UART_Init()` paths for the user device framework.
- Keep trace and legacy core UART paths intact while allowing the GPS UART to be
  initialized through `dev_uart_init()` and port-owned MSP resource setup.

**Files Changed**

- `Core/Src/usart.c` - Added USER CODE hooks so USART1 MSP init/deinit delegates
  to the user port adapter and returns before the generated USART1 MSP block.
- `Usr/port/usr_port_hal.h` - Kept shared HAL adapter declarations narrow and
  removed UART runtime/DMA ring details from the public shared port header.
- `Usr/port/port_bsp/port_hal_uart.h` - Added the UART-specific STM32 port
  context that binds a HAL UART handle to optional RX DMA ring state.
- `Usr/port/port_bsp/port_hal_uart.c` - Added a private USART1 resource table,
  MSP init/deinit implementation, UART config application, restart handling,
  and RX DMA state management behind `dev_uart_ops_t`.
- `Usr/port/board_support.c` - Bound USART1, USART2, and LPUART1 through opaque
  UART port contexts while only USART1 owns the GPS RX DMA ring.

**API / Protocol Impact**

- `dev_uart_ops_t` and the public `dev_uart_*` API are unchanged.
- `uart_device_t` continues to expose a generic `ctx + config + ops` model;
  reusable UART code no longer needs to carry STM32 HAL handles or RX ring
  fields directly.
- UART runtime changes still go through `dev_uart_control()` commands such as
  baudrate, parity, stop bits, word length, start RX, and stop RX.

**Layering / Dependency Impact**

- STM32 HAL types and HAL calls remain in `Usr/port` or legacy `Core` services.
- GPS USART1 pins, DMA channel, DMA request, and HAL handle binding are now
  port-owned resources rather than reusable driver-layer fields.
- Trace paths in `Core/Src/usart_if.c` were not modified; LPUART1/USART2/RTT
  behavior remains legacy-owned.

**Verification**

- `arm-none-eabi-gcc ... -fsyntax-only` for `Usr/port/port_bsp/port_hal_uart.c`,
  `Usr/port/board_support.c`, and `Core/Src/usart.c`: exit code 0 for all three;
  existing `Core/Inc/main.h` RTC macro redefinition warnings remain.
- `arm-none-eabi-gcc ... -fsyntax-only` for `Core/Src/usart_if.c`: exit code 0;
  existing `Core/Inc/main.h` RTC macro redefinition warnings remain.
- `[local tool path] -C STM32CubeIDE/Debug all`: blocked
  before source compilation because the available GCC rejects the CubeIDE 14.3
  flag `-fcyclomatic-complexity`.

**Risks / Follow-up**

- Full firmware build still requires the STM32CubeIDE GNU Tools for STM32
  14.3.rel1 toolchain or equivalent support for the generated makefile flags.
- Hardware validation is still required for GPS USART1 RX DMA reception,
  runtime baudrate/control changes, and trace output after board reset.
- The USART1 resource table is intentionally scoped to the current GPS path;
  add more UART resources only when another user-framework UART requires port
  ownership.

<!-- END_SOURCE_FRAGMENT S017-F6 -->

## Source lines 951-997

<!-- SOURCE_FRAGMENT S017-F7 -->
## 2026-09-06 - repository agent prompt and framework rules refresh

**Purpose**

- Convert the repository-level engineering contract into an explicit agent
  system prompt aligned with the current `Usr/` layering model and the
  cooperative bare-metal execution model.
- Capture the current rule set for coding style, function design, framework
  layering, embedded constraints, verification, and forbidden practices in a
  single authoritative repository file.

**Files Changed**

- `AGENTS.md` - Replaced the generic repository guidance with a repository-
  scoped agent prompt covering agent content, coding rules, function design,
  layering, driver framework design, verification, embedded rules, and
  forbidden rules.

**API / Protocol Impact**

- No firmware API, protocol, or command format changed.
- The change affects the engineering contract used to author future firmware
  edits.

**Layering / Dependency Impact**

- HAL/LL/vendor dependencies remain isolated to `Usr/port` for the reusable
  user framework.
- The prompt now states the current ownership split for `Usr/app`,
  `Usr/device`, `Usr/drivers`, and `Usr/port`, and preserves the existing
  trace path and legacy CubeMX core services.

**Verification**

- Pending: repository build and hardware verification are not required for this
  documentation-only change, but the updated contract should be checked by
  future architecture or driver changes with `git diff --check` and the normal
  build command.

**Risks / Follow-up**

- The repository now has a stricter engineering contract in `AGENTS.md`; future
  driver work must follow it or the change log will drift from the code.
- `Usr/DEVICE_DRIVER_FRAMEWORK.md` still contains older wording and should be
  aligned with the new prompt in a follow-up documentation pass if it is kept
  as a developer reference.

<!-- END_SOURCE_FRAGMENT S017-F7 -->
