# Historical GPIO, I2C and DMA design

> Authority: historical source excerpt, not current API or current verification.
> Source: S008, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/specs/2026-09-17-gpio-i2c-dma-resource-decoupling-design.md`.

Hardware mapping is old KG200Z context; it is not a portable board definition.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-69

<!-- SOURCE_FRAGMENT S008-F1 -->
# GPIO, I2C, and DMA Resource Decoupling Design

## Goal

Remove the active firmware dependency on `MX_GPIO_Init()`, `MX_I2C1_Init()`,
and `MX_DMA_Init()` while preserving current board behavior. Ownership follows
the resource consumer, not the peripheral category.

## Ownership Model

### GPIO

The user framework owns these logical resources through the board binding and
`port_hal_gpio`:

| Logical resource | Physical binding | Initial configuration |
| --- | --- | --- |
| `USR_PIN_GNSS_PW_EN` | PA8 | output push-pull, pull-down, low |
| `USR_PIN_GNSS_RST` | PA12 | output push-pull, pull-up, low |
| `USR_PIN_GNSS_STANDBY` | PA0 | output push-pull, pull-down, low |
| `USR_PIN_ICP_INT` | PB1 | falling-edge EXTI, pull-up, EXTI1 priority 3 |
| `USR_PIN_PRIMARY_SPI_CS` | PA4 | output push-pull, no pull, high |
| `USR_PIN_SECONDARY_SPI_CS` | PB12 | output push-pull, no pull, high |
| `USR_PIN_WDOG` | PA1 | output push-pull, no pull, low |

Each descriptor carries the GPIO clock callback, mode, pull, speed, initial
level, and optional IRQ metadata. Output levels are written before switching
the pins to output mode so chip-select and GNSS control pins do not glitch.

The Core legacy GPIO service retains only resources consumed outside the user
framework: LoRa LED2 on PB9 and the existing PB5/PC0/PC1/PC4 safe-state pins.
It is renamed to a Core-owned entry point and is not called through an `MX_*`
API. The EXTI1 vector remains a short Core shell and forwards the IRQ number to
the port adapter; it does not name `ICP20_INT_Pin`.

### I2C1

I2C1 is owned by the sensor bus port adapter. `port_hal_i2c` owns:

- the private `I2C_HandleTypeDef`;
- the I2C1 instance and PCLK1 kernel-clock selection;
- PB6/PB7 AF4 open-drain GPIO resources;
- timing `0x20309DEA`, 7-bit addressing, analog filter enabled, and digital
  filter value 0;
- HAL init/deinit and bus recovery.

The application composition root explicitly calls `dev_i2c_init()` before the
HDC1080 driver uses the bus. Core `i2c.c` remains only as the HAL-required MSP
dispatcher. Board binding receives the opaque context from
`usr_port_hal_i2c_get_context()` and no longer includes `i2c.h` or names
`hi2c1`. The legacy trace file loses its unrelated I2C reset/resume branches.

### DMA

DMA remains private to each consumer. No public DMA device or registry is
introduced.

| DMA resource | Owner |
| --- | --- |
| DMA1 Channel1 | Core ADC MSP |
| DMA1 Channel2 | port USART1 RX |
| DMA1 Channel3 | port USART2 RX |
| DMA1 Channel6 | port LPUART1 RX |

DMA1 and DMAMUX1 controller clocks are shared infrastructure. Each active DMA
consumer enables those clocks idempotently immediately before configuring its
channel. Consumer deinit does not disable the shared controller clocks. The
active startup and trace paths no longer call `MX_DMA_Init()`.

<!-- END_SOURCE_FRAGMENT S008-F1 -->

## Source lines 88-110

<!-- SOURCE_FRAGMENT S008-F2 -->
## Compatibility And Non-goals

- Reusable `Usr/drivers` and `Usr/device` code must not include STM32 headers.
- Existing GPIO, SPI, UART, I2C transaction APIs remain static and heap-free.
- I2C remains polling based. No I2C DMA or event/error IRQ is added.
- SPI remains polling based.
- The `.ioc` file is not an ownership authority for this migration because it
  does not describe several active source-level resources.
- Hardware validation is required after build verification for GPIO default
  levels, ICP EXTI, HDC1080 access, UART RX DMA, and ADC DMA.

## Acceptance Criteria

- No active or declared `MX_GPIO_Init`, `MX_I2C1_Init`, or `MX_DMA_Init` symbol
  remains in `Core` or `Usr`.
- `hi2c1` and the platform I2C reinit callback are removed.
- Port GPIO initialization is descriptor driven and preserves all seven
  logical resource defaults.
- I2C1 initialization, filters, MSP, private handle, and recovery are port
  owned.
- ADC and UART DMA consumers enable controller clocks before `HAL_DMA_Init()`.
- Static regression checks, `git diff --check`, and the supported firmware
  build pass without new warnings.
<!-- END_SOURCE_FRAGMENT S008-F2 -->
