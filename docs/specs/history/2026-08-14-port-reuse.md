# Historical reusable port design

> Authority: historical source excerpt, not current API or current verification.
> Source: S007, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/specs/2026-08-14-usr-port-reuse-design.md`.

Bus/port boundaries and error mapping. MX-only configuration ownership is historical.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-64

<!-- SOURCE_FRAGMENT S007-F1 -->
# Reusable STM32 HAL Port Design

## Goal

Make one STM32 HAL adapter implementation reusable with any compatible UART,
I2C, SPI, GPIO, and time-service instance. Board-specific handles, pins,
buffers, clocks, IRQs, and device choices must be declared in one board binding.
The platform-independent bus and device drivers must remain unchanged when a
new STM32 peripheral instance or board is introduced. Supporting another MCU
requires replacing only `Usr/port`.

## Selected Architecture

The implementation is divided into three boundaries:

1. `Usr/drivers/bus` defines platform-independent operations and owns generic
   policies such as locking, retry, timeout defaults, and byte buffering.
2. `Usr/port/usr_port_hal_*` implements shared STM32 HAL adapters. Each adapter
   receives all instance state through a context; these files contain no
   `huart1`, `hi2c1`, `hspi1`, physical pin, IRQ, or board-role references.
3. A KG200Z board binding under `Usr/port` owns static contexts and storage,
   binds generated HAL handles to the shared operation tables, and exports one
   typed port bundle to `Usr/app/usr_app.c`.

This keeps runtime cost at static allocation plus function-pointer dispatch.
There is no registry, heap allocation, name lookup, or device-tree parser.

## Interfaces and Ownership

CubeMX 的 `.ioc` 文件和生成的 `MX_*_Init()` 独占外设配置。共享 HAL 适配器只能读取已生成的 `Init` 值并执行运行时事务，不能写 `handle->Init`，不能调用 UART/SPI/I2C/GPIO 初始化，也不能配置 NVIC。平台入口可以调用生成的 MX 函数；板级 I2C 恢复回调可以先结束当前 HAL 生命周期，再调用完整 `MX_I2C1_Init()`。

Each bus type gains a typed backend value containing its matching `ops` and
`ctx`. Passing one value instead of two unrelated arguments prevents pairing
an operation table with the wrong context. A shared constant operation table
exists once per STM32 HAL peripheral class; contexts hold per-instance state.

The UART HAL context owns a HAL handle pointer, caller-provided DMA storage,
storage size, read position, and reception state. The generic UART
bus exposes a nonblocking byte stream. NMEA line assembly moves to the GPS
driver because line framing is a GPS protocol concern, not a UART capability.
Buffer sizes become board/device configuration rather than public bus macros.

The I2C HAL context owns only the HAL handle and HAL-specific probe settings.
Timeout and recovery retry policy belong to the bus configuration. Register
access validates 7-bit addresses and one- or two-byte register widths. The ICP
driver passes its configured timeout to the bus instead of carrying an unused
field. Unsupported register byte-order options are removed until a concrete
device requires them.

The SPI HAL context receives its actual kernel clock frequency. The adapter
derives the actual speed from the MX-owned prescaler and validates the device's
maximum speed, mode, bit order, and 8-bit data width without changing any SPI
register. Transfer dispatch supports full-duplex, transmit-only, and
receive-only calls according to which buffers are present. Chip select and
timeout remain runtime/device policy.

The GPIO HAL context receives a descriptor array and callback storage. CubeMX
owns GPIO modes, alternate functions, EXTI edges, and NVIC priority/enable
state; the adapter only writes/reads pins and records callbacks. The generated
IRQ entry forwards HAL EXTI notifications to the reusable dispatcher.

Time services become a typed `ops + ctx` backend so fake clocks and alternative
time sources can be injected without global functions.

<!-- END_SOURCE_FRAGMENT S007-F1 -->

## Source lines 79-92

<!-- SOURCE_FRAGMENT S007-F2 -->
## Error Handling

Every adapter validates its context, handle, buffers, sizes, and required HAL
state before use. HAL status mapping is consistent across UART, I2C, and SPI:
timeout maps to `USR_ERR_TIMEOUT`, busy to `USR_BUSY`, invalid input to
`USR_ERR_PARAM`, and other HAL failures to `USR_ERR_BUS`. Initialization stops
when a required backend fails; an absent optional ICP remains nonfatal and is
reported through the existing log path. Recovery failures are preserved and
never cleared before a successful transaction.

GPIO read/write operations report invalid logical pins rather than silently
returning zero or dropping writes. UART overflow accounting uses producer and
consumer progress instead of treating a particular modulo index as overflow.

<!-- END_SOURCE_FRAGMENT S007-F2 -->
