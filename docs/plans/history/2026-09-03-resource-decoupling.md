# Historical resource decoupling proposal

> Authority: historical source excerpt, not current API or current verification.
> Source: S002, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/plans/2026-09-03-rtthread-style-port-resource-decoupling.md`.

Architecture, private descriptors and reusable wrappers only. RT-Thread was a reference model, not an RTOS dependency.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-136

<!-- SOURCE_FRAGMENT S002-F1 -->
# RT-Thread Style Port Resource Decoupling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rework the KG200Z `Usr` device framework so UART, SPI, I2C, GPIO, DMA, timer, and future ADC resources follow an RT-Thread-like split: public bus/device layers expose abstract operations, while STM32 HAL handles, pinmux, clocks, DMA channels, and IRQ ownership stay private to `Usr/port`.

**Architecture:** DMA is not promoted to a public device used by GPS/ICP/FRAM drivers. Each peripheral adapter owns the DMA channels it needs through port-private descriptors and runtime contexts. `Usr/drivers/bus/dev` keeps only reusable bus semantics; `Usr/port/port_bsp` owns board hardware descriptors, HAL init/deinit, DMA link, and IRQ forwarding.

**Tech Stack:** STM32WLE5 HAL, C11 bare-metal firmware, STM32CubeIDE generated project, static allocation, no RTOS dependency.

**Spec:** Chat design request on 2026-09-03: apply RT-Thread-style decoupling to DMA and other modules.

## Global Constraints

- `Usr/port` is the only layer allowed to include STM32 HAL/LL/vendor peripheral headers.
- Do not expose `UART_HandleTypeDef`, `SPI_HandleTypeDef`, `I2C_HandleTypeDef`, `DMA_HandleTypeDef`, `GPIO_TypeDef`, `USART_TypeDef`, `SPI_TypeDef`, or `I2C_TypeDef` through `Usr/drivers` or `Usr/device` public headers.
- Do not hardcode `huart1`, `hspi1`, `hi2c1`, GPIO ports, physical pins, DMA channels, or IRQ numbers in reusable bus/device code.
- Keep trace and LoRaWAN Core services on their current CubeMX-owned path unless a separate task explicitly migrates them.
- All migrated resources must remain statically allocated; no heap, registry parser, or runtime device-tree mechanism.
- Every non-trivial source change must update `docs/change_log/CHANGELOG.md` with purpose, files changed, API impact, dependency-boundary impact, verification, risk, and hardware status.

---

## RT-Thread Reference Model for This Project

RT-Thread does not make UART GPIO and DMA part of the public serial object. The public serial/SPI/I2C device exposes configuration and operations; BSP/private driver data owns the hardware binding. The same model maps to this project as:

| Concept | Public layer | Port-private layer |
| --- | --- | --- |
| UART | byte stream ops, abstract baud/format request | USART instance, TX/RX AF pins, UART HAL handle, RX DMA handle, IRQ |
| SPI | transfer ops, mode/max-speed/data-width request | SPI instance, SCK/MISO/MOSI AF pins, optional TX/RX DMA, IRQ |
| I2C | register/master ops, timing capability request | I2C instance, SCL/SDA AF/open-drain pins, recovery callback, optional DMA |
| GPIO | logical pin IDs for runtime read/write/IRQ callback | physical port/pin, mode/pull/speed, EXTI/NVIC configuration |
| DMA | no public bus/device object for chip drivers | channel descriptor, handle storage, owner claim, HAL link, IRQ dispatch |
| Timer | tick/start/stop ops | TIM instance, clock/NVIC, HAL or LL timer state |
| ADC | sampling ops if needed later | ADC instance, channels, DMA, IRQ, calibration sequence |

The important rule: DMA is a resource dependency of a concrete port adapter, not a reusable upper-layer service.

## Current Boundary Problems to Correct

- `Usr/drivers/bus/dev/dev_uart.h` currently exposes `handle`, `dma`, `ring`, and a config shape that is too close to HAL values.
- `Usr/drivers/bus/dev/dev_spi.h` mixes SPI bus lifetime with GPIO CS ownership; this is acceptable only because it uses logical GPIO ops, but it should be reviewed before adding more SPI devices.
- `Usr/drivers/bus/dev/dev_gpio.h` depends on `gpio_pin_t`, which is a port-style physical pin descriptor and should not leak below `Usr/port`.
- `Core/Src/dma.c` globally enables all DMA IRQs, while `Core/Src/stm32wlxx_it.c` directly calls `HAL_DMA_IRQHandler(&hdma_*)` for generated global DMA handles.
- `Core/Src/usart.c`, `Core/Src/spi.c`, `Core/Src/i2c.c`, and `Core/Src/adc.c` still own many MSP bindings. Some are legacy Core services, but custom `Usr` devices should move toward port-private ownership.
- `Core/Src/usart_if.c` still calls USART/DMA MX functions behind feature macros; those paths must be classified before removing or keeping them.

## Target File Responsibility Map

- Modify: `Usr/drivers/bus/dev/dev_uart.h` and `.c` - public UART stream object; abstract config only; no HAL/ring/DMA fields.
- Modify: `Usr/drivers/bus/dev/dev_spi.h` and `.c` - public SPI transfer object; no HAL handle; decide whether CS remains a logical GPIO operation or moves into a per-slave wrapper.
- Modify: `Usr/drivers/bus/dev/dev_i2c.h` and `.c` - public I2C master object; no HAL handle; optional abstract capability validation.
- Modify: `Usr/drivers/bus/dev/dev_gpio.h` and `.c` - public logical GPIO object; no physical pin descriptor exposure.
- Modify: `Usr/drivers/bus/dev/dev_timer.h` and `.c` - public timer/tick object; no HAL handle in the public struct.
- Create: `Usr/port/port_bsp/port_hal_dma.h` and `.c` - private DMA descriptor helpers, channel ownership checks, and IRQ dispatch helpers.
- Create: `Usr/port/port_bsp/port_hal_resource.h` - private shared descriptor types for clocks, AF pins, IRQ priority, and optional DMA.
- Modify: `Usr/port/port_bsp/port_hal_uart.c` - UART HAL adapter owns `UART_HandleTypeDef`, RX DMA, ring storage, pinmux, clock, and IRQ handling through private context.
- Modify: `Usr/port/port_bsp/port_hal_spi.c` - SPI HAL adapter owns `SPI_HandleTypeDef`, pinmux, clock, and optional DMA.
- Modify: `Usr/port/port_bsp/port_hal_i2c.c` - I2C HAL adapter owns `I2C_HandleTypeDef`, pinmux, clock, and recovery.
- Modify: `Usr/port/port_bsp/port_hal_gpio.c` - GPIO HAL adapter owns physical pin descriptors and EXTI callback dispatch.
- Modify: `Usr/port/board_support.c` - KG200Z board resource table; exposes only bus/device objects to `Usr/app`.
- Modify: `Core/Src/stm32wlxx_it.c` - forward migrated custom IRQs to `Usr/port`; keep legacy Core IRQs generated/direct where not migrated.
- Modify: `Core/Src/usart_if.c` - remove or guard custom USART1/GPS initialization calls after USART1 is port-owned.
- Modify: `docs/change_log/CHANGELOG.md` - record each migration slice.

## Private Descriptor Shape

Use small descriptors instead of one large mixed `resource` struct. The names can change during implementation, but the split should remain.

```c
typedef void (*usr_port_clock_fn_t)(void);

typedef struct
{
  usr_port_clock_fn_t enable;
  usr_port_clock_fn_t disable;
} usr_port_clock_ctl_t;

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  uint32_t mode;
  uint32_t pull;
  uint32_t speed;
  uint32_t alternate;
  const usr_port_clock_ctl_t *clock;
} usr_port_gpio_hw_t;

typedef struct
{
  DMA_Channel_TypeDef *channel;
  uint32_t request;
  uint32_t direction;
  uint32_t periph_inc;
  uint32_t mem_inc;
  uint32_t periph_alignment;
  uint32_t mem_alignment;
  uint32_t mode;
  uint32_t priority;
  IRQn_Type irq;
  uint32_t irq_preempt_prio;
  uint32_t irq_sub_prio;
} usr_port_dma_hw_t;

typedef struct
{
  USART_TypeDef *instance;
  const usr_port_clock_ctl_t *clock;
  const usr_port_gpio_hw_t *tx;
  const usr_port_gpio_hw_t *rx;
  const usr_port_dma_hw_t *rx_dma;
  IRQn_Type irq;
  uint32_t irq_preempt_prio;
  uint32_t irq_sub_prio;
} usr_port_uart_hw_t;
```

Runtime context stays separate:

```c
typedef struct
{
  const usr_port_uart_hw_t *hw;
  UART_HandleTypeDef hal;
  DMA_HandleTypeDef rx_dma;
  Ring_dma_t ring;
  uint8_t *rx_buffer;
  uint16_t rx_buffer_size;
  bool rx_started;
} usr_port_uart_ctx_t;
```

This keeps the static board description readable and keeps runtime state from becoming a board-configuration dump.

<!-- END_SOURCE_FRAGMENT S002-F1 -->

## Source lines 179-294

<!-- SOURCE_FRAGMENT S002-F2 -->
### Task 2: Introduce Port-Private Resource Types

**Files:**
- Create: `Usr/port/port_bsp/port_hal_resource.h`
- Create: `Usr/port/port_bsp/port_hal_dma.h`
- Create: `Usr/port/port_bsp/port_hal_dma.c`
- Modify: `STM32CubeIDE/Debug/Usr/port/port_bsp/subdir.mk` if the generated project requires manual source registration
- Modify: `docs/change_log/CHANGELOG.md`

**Interfaces:**
- Consumes: no public bus API changes yet.
- Produces: private descriptor helpers consumed by UART/SPI/I2C/ADC adapters.

- [ ] **Step 1: Add shared hardware descriptor types**

Add `usr_port_clock_ctl_t`, `usr_port_gpio_hw_t`, `usr_port_irq_hw_t`, and `usr_port_dma_hw_t` to `port_hal_resource.h`. This header may include `stm32wlxx_hal.h` because it lives under `Usr/port`.

- [ ] **Step 2: Add DMA helper functions**

`port_hal_dma.h` should declare:

```c
usr_status_t usr_port_dma_init(DMA_HandleTypeDef *handle,
                               const usr_port_dma_hw_t *hw);
usr_status_t usr_port_dma_deinit(DMA_HandleTypeDef *handle);
void usr_port_dma_irq(DMA_HandleTypeDef *handle);
```

`usr_port_dma_init()` fills `handle->Instance` and `handle->Init`, enables DMAMUX/DMA clocks once, calls `HAL_DMA_Init()`, and configures/enables only the IRQ owned by that DMA descriptor.

- [ ] **Step 3: Keep HAL link in the peripheral adapter**

Do not try to make `__HAL_LINKDMA()` fully generic. The field name is peripheral-specific, so UART/SPI/ADC adapters call it directly:

```c
__HAL_LINKDMA(&ctx->hal, hdmarx, ctx->rx_dma);
```

This is still decoupled because both handles are private to `Usr/port`.

- [ ] **Step 4: Verify helper compiles standalone**

Run syntax checks if the cross compiler is available:

```powershell
Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue
```

If present, run targeted `-fsyntax-only` checks for the new files using the same include set as existing port files. If absent, record that full compile is deferred to STM32CubeIDE.

### Task 3: Shrink Public Bus Device Objects

**Files:**
- Modify: `Usr/drivers/bus/dev/dev_uart.h`
- Modify: `Usr/drivers/bus/dev/dev_uart.c`
- Modify: `Usr/drivers/bus/dev/dev_spi.h`
- Modify: `Usr/drivers/bus/dev/dev_spi.c`
- Modify: `Usr/drivers/bus/dev/dev_i2c.h`
- Modify: `Usr/drivers/bus/dev/dev_i2c.c`
- Modify: `Usr/drivers/bus/dev/dev_gpio.h`
- Modify: `Usr/drivers/bus/dev/dev_gpio.c`
- Modify: `Usr/drivers/bus/dev/dev_timer.h`
- Modify: `Usr/drivers/bus/dev/dev_timer.c`
- Modify: `Usr/drivers/bus/dev/dri_ops.h`
- Modify: all `Usr/drivers/dev` and `Usr/app` call sites needed for signature changes
- Modify: `docs/change_log/CHANGELOG.md`

**Interfaces:**
- Consumes: existing `ops` forwarding pattern.
- Produces: public structs that carry `ops`, `ctx`, and abstract config only.

- [ ] **Step 1: Replace HAL-like public fields**

Change public bus structs toward this shape:

```c
typedef struct
{
  const dev_uart_ops_t *ops;
  void *ctx;
  uart_device_config_t config;
} uart_device_t;
```

Use the same pattern for SPI/I2C/GPIO/timer. Public code passes `dev->ctx` to the op implementation, not the public wrapper itself, unless the operation intentionally needs wrapper metadata.

- [ ] **Step 2: Make UART config abstract**

Replace HAL numeric fields such as `UART_WORDLENGTH_8B` and `UART_STOPBITS_1` in public config with project enums:

```c
typedef enum { DEV_UART_DATA_BITS_8 = 8u } dev_uart_data_bits_t;
typedef enum { DEV_UART_STOP_BITS_1 = 1u } dev_uart_stop_bits_t;
typedef enum { DEV_UART_PARITY_NONE = 0u, DEV_UART_PARITY_EVEN, DEV_UART_PARITY_ODD } dev_uart_parity_t;
```

The port adapter maps these to HAL values.

- [ ] **Step 3: Remove physical GPIO descriptors from public GPIO**

`gpio_device_t` should not expose `gpio_pin_t *pins`. It should expose only logical operations and `ctx`:

```c
typedef struct
{
  const dev_gpio_ops_t *ops;
  void *ctx;
} gpio_device_t;
```

The physical pin array moves to a private `usr_port_gpio_ctx_t` in `Usr/port`.

- [ ] **Step 4: Decide SPI CS ownership before editing code**

Recommended option for this project: keep CS as logical GPIO at the bus transaction boundary for now because existing FRAM/ICP code already uses `dev_spi_begin/frame/end`. Do not put physical CS GPIO in SPI resource. A later SPI-slave wrapper can be added only if multiple devices on one bus need per-device mode/speed state.

<!-- END_SOURCE_FRAGMENT S002-F2 -->

## Source lines 440-459

<!-- SOURCE_FRAGMENT S002-F3 -->
### Task 8: Timer and ADC Policy

**Files:**
- Modify: `Usr/port/port_bsp/port_hal_tim.c`
- Modify: `Usr/drivers/bus/dev/dev_timer.*`
- Create or modify later: `Usr/drivers/bus/dev/dev_adc.*` only when a Usr-owned ADC consumer exists
- Modify: `docs/change_log/CHANGELOG.md`

**Interfaces:**
- Consumes: current timer ops and future ADC requirements.
- Produces: consistent policy for non-communication peripherals.

- [ ] **Step 1: Keep TIM2 private but align the struct shape**

Move `TIM_HandleTypeDef *handle` out of public `timer_device_t` and into a private `usr_port_tim_ctx_t`.

- [ ] **Step 2: Defer ADC migration until ownership is clear**

ADC currently has Core-generated DMA and IRQ binding. Do not migrate it with UART/SPI/I2C unless a `Usr` device consumes it. When migrated, ADC follows the same private descriptor model: ADC instance, channels, calibration, DMA, IRQ, buffer, and conversion policy remain in `Usr/port`.

<!-- END_SOURCE_FRAGMENT S002-F3 -->
