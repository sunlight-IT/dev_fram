# Historical semantic timer and PWM proposal

> Authority: historical source excerpt, not current API or current verification.
> Source: S006, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/plans/2026-09-30-semantic-timer-pwm-resource-architecture.md`.

Capture, claims and calculators were subsequently removed. Excludes the application-integration task.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-85

<!-- SOURCE_FRAGMENT S006-F1 -->
# Semantic Timer and PWM Resource Architecture Implementation Plan

> **For agentic workers:** Execute this plan task by task. Preserve the existing dependency direction and run the required review and verification gates after each task.

**Goal:** Move UART/SPI/TIM/PWM configuration to application-semantic parameters, add a reusable TIM basic/capture port flow, and add a TIM-backed PWM device with explicit static resource ownership.

**Architecture:** `Usr/app` and device drivers provide portable semantic configuration. `Usr/port` owns STM32 HAL conversion, clock discovery, PSC/ARR/CCR calculation, GPIO/IRQ resources, resource claims, lifecycle, and error mapping. TIM2 remains exclusively owned by the Usr scheduler's 1 ms tick; PWM and capture use separate validated board resources only when a real board row exists.

**Tech Stack:** C11, STM32WLE5JC HAL, bare-metal cooperative scheduler, static allocation, Python structural/calculator tests, repository `scripts/build.py` and `scripts/flash.py`.

## Global Constraints

- Preserve `Usr/app -> Usr/device -> Usr/drivers -> Usr/port -> HAL/hardware`.
- `Usr/drivers` and `Usr/device` must not include HAL/vendor headers or expose GPIO, IRQ, DMA, timer channel, HAL handle, or physical peripheral types.
- TIM2 remains the sole Usr scheduler 1 ms owner. RTC `TIMER_IF` remains the `HAL_GetTick()`/`HAL_Delay()` timebase. Do not enable the HAL TIM timebase template or add another TIM2 owner.
- Use static resource tables and runtime contexts. No heap allocation and no public `dma_device_t`.
- Preserve default initialization. `NULL` semantic configuration selects the bound instance default; explicit configuration is copied into runtime state and never mutates caller storage.
- A TIM/PWM/capture resource without a confirmed board `.ioc`/pin row is disabled and returns `USR_ERR_UNSUPPORTED`; it must not be started during normal boot or represented by a fake device.
- ISR work is bounded: clear/dispatch flags, capture raw events, publish update events; no protocol parsing, logging, blocking, or application state-machine work in ISR.
- Separate structural checks, semantic calculator tests, firmware build, flash verification, and live RTT/register/peripheral evidence.

## Task 1: Freeze ownership and semantic contracts

**Files:**
- Modify: `Usr/drivers/bus/dev/dri_ops.h`, `Usr/drivers/bus/dev/dev_uart.h`, `Usr/drivers/bus/dev/dev_spi.h`, `Usr/drivers/bus/dev/dev_timer.h`
- Create: `Usr/drivers/bus/dev/dev_pwm.h`, `Usr/drivers/bus/dev/dev_pwm.c`
- Modify: `Usr/drivers/bus/dev/dev_uart.c`, `Usr/drivers/bus/dev/dev_spi.c`, `Usr/drivers/bus/dev/dev_timer.c`
- Modify: `scripts/peripheral_resource_structure_test.py`

Define HAL-free semantic enums and structures for UART data bits/parity/stop bits/direction, SPI speed/mode/bit order/data width, timer basic/capture mode, capture edge/filter/timeout, and PWM frequency/duty_permille/polarity. Define lifecycle/status behavior, NULL/default behavior, units, pointer rules, and output ownership. Add PWM forwarding APIs with no vendor symbols. Add structural checks proving no HAL include/type/constant appears in `Usr/drivers` or `Usr/device`, and proving semantic units/enums exist.

Required PWM public behavior: `init`, `deinit`, `start`, `stop`, `set_duty`, `set_frequency`, `get_actual`, and `get_status`. `set_duty` preserves frequency; `set_frequency` preserves duty_permille and returns actual values/quantization status.

## Task 2: Migrate UART/SPI semantic configuration

**Files:**
- Modify: `Usr/port/port_bsp/port_hal_uart.h`, `Usr/port/port_bsp/port_hal_uart.c`
- Modify: `Usr/port/port_bsp/port_hal_spi.h`, `Usr/port/port_bsp/port_hal_spi.c`
- Modify: `Usr/port/board_support.c`, `Usr/app/usr_app.c`, `Usr/app/usr_app_config.h`

Map semantic UART/SPI values to HAL values only inside port. Preserve current defaults and actual current board behavior. UART control must save old semantic config, stop RX, apply/reinitialize, restart RX, and restore the old configuration/state if any step fails. SPI must derive/validate prescaler from the actual board kernel clock and reject incompatible speed/mode/width without silently changing device policy. Add structural checks that drivers contain no HAL constants and port owns all mappings.

## Task 3: Implement timer resource/context, calculator, and scheduler ownership

**Files:**
- Modify: `Usr/port/usr_port_hal.h`, `Usr/port/port_bsp/port_hal_tim.h`, `Usr/port/port_bsp/port_hal_tim.c`
- Modify: `Usr/port/board_support.c`, `Usr/port/board_support.h`
- Modify: `Usr/drivers/bus/dev/dev_timer.h`, `Usr/drivers/bus/dev/dev_timer.c`, `Usr/app/usr_app.c`
- Modify: `Core/Src/sys_app.c`, `CMakeLists.txt`, `Core/Src/stm32wlxx_it.c`, `Core/Inc/stm32wlxx_it.h`
- Modify: `scripts/peripheral_resource_structure_test.py`
- Create: `scripts/timer_semantic_test.py`

Replace the hardcoded single timer context with immutable resource descriptors and mutable instances. Descriptor fields must cover TIM instance, clock getter/gate, counter width, update/capture IRQ metadata, channel capability, GPIO/AF capability, owner/reserved role, and conflict domain. Context fields must cover handle, semantic config, derived counter_hz/PSC/ARR/actual period, lifecycle state, claim mask, callbacks, event storage, and error counters.

Implement a 64-bit calculator that discovers the real timer clock, selects PSC/ARR within 16/32-bit limits, minimizes quantization error, and returns actual values. Implement validate -> claim -> clock -> calculate -> HAL init -> IRQ -> ready with reverse rollback. Define idempotent/rejected lifecycle calls and safe retry after failure.

TIM2 is a scheduler-only resource. Add checks that the build does not include the HAL TIM timebase template, that only one TIM2 IRQ/MSP/clock owner exists, and that RTC remains the HAL timebase. Do not allow TIM2 to be claimed by PWM/capture.

The calculator tests must cover 48 MHz/1 ms, other periods, 16/32-bit limits, overflow, quantization, unsupported values, and scheduler/TIM2 reservation.

## Task 4: Add capture event path

**Files:**
- Modify: `Usr/port/port_bsp/port_hal_tim.h`, `Usr/port/port_bsp/port_hal_tim.c`
- Modify: `Usr/drivers/bus/dev/dev_timer.h`, `Usr/drivers/bus/dev/dev_timer.c`
- Modify: `Usr/port/board_support.c`, `Core/Src/stm32wlxx_it.c`, `Core/Inc/stm32wlxx_it.h`
- Modify: `scripts/timer_semantic_test.py`, `scripts/peripheral_resource_structure_test.py`

Add semantic capture initialization and capability validation. Define a fixed-size static event containing sequence, raw counter, update epoch, channel, edge, overcapture, and update-pending metadata. ISR is producer; foreground `process/read` is consumer. Ring-full policy is drop-new with observable dropped_count. First valid edge establishes a baseline only. Foreground processing handles counter-width wrap, missing edge pairing, timeout, sequence gaps, overcapture, and returns application time units rather than CCR/channel/HAL values.

Each enabled TIM vector must have one dispatcher owner. Unbound TIM1/TIM16/TIM17 vectors remain disabled/unsupported until a real board row is added.

## Task 5: Add PWM resource/device path

**Files:**
- Modify: `Usr/port/port_bsp/port_hal_tim.h`, `Usr/port/port_bsp/port_hal_tim.c`
- Create or modify: `Usr/port/port_bsp/port_hal_pwm.h`, `Usr/port/port_bsp/port_hal_pwm.c`
- Modify: `Usr/port/board_support.c`, `Usr/port/board_support.h`
- Modify: `Usr/drivers/bus/dev/dev_pwm.h`, `Usr/drivers/bus/dev/dev_pwm.c`
- Modify: `scripts/timer_semantic_test.py`, `scripts/peripheral_resource_structure_test.py`

Bind PWM to an injected timer resource/channel descriptor. Same-TIM channels share PSC/ARR. Store semantic frequency and duty for every active channel. `set_duty` writes only CCR. `set_frequency` must precompute the new base and every active CCR, reject incompatible channels without register changes, and commit the complete update as one coordinated operation; on HAL failure restore the previous base/channel state. Return actual frequency, duty, ARR, CCR, and errors. Duty calculation is `round((ARR+1)*duty_permille/1000)` clamped to ARR.

Do not add unverified physical PWM/capture pins to `.ioc`, Core generated files, or board binding. An empty/disabled table must compile and return `USR_ERR_UNSUPPORTED` without boot side effects. Add a fixture-only valid descriptor for calculator/claim tests if needed, clearly separated from live board resources.

<!-- END_SOURCE_FRAGMENT S006-F1 -->
