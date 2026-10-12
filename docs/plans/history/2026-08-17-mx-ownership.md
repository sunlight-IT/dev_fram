# Historical MX ownership proposal

> Authority: historical source excerpt, not current API or current verification.
> Source: S001, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/plans/2026-08-17-mx-peripheral-configuration-ownership.md`.

Superseded ownership proposal. Selected UART/SPI/I2C/GPIO contracts; omitted application and chip-driver changes.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-40

<!-- SOURCE_FRAGMENT S001-F1 -->
# MX Peripheral Configuration Ownership Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让 `KG200Z_OPEN.ioc` 与 CubeMX 生成的 `MX_*_Init()` 独占 UART、SPI、I2C、GPIO、DMA、时钟和 NVIC 配置，`Usr/port` 只绑定实例、校验设备需求并执行运行时操作。

**Architecture:** `.ioc` 是唯一配置源，`usr_port_platform_init()` 只负责按顺序调用生成的初始化函数；共享 HAL 适配器不得写 `handle->Init`，也不得自行执行外设或 GPIO/NVIC 配置。应用和设备驱动继续声明所需能力，但 `set_cfg` 改为 `validate_cfg`；I2C 故障恢复通过注入的板级回调重新执行完整 MX 初始化流程。

**Tech Stack:** STM32CubeMX, STM32WL HAL, C11, Keil MDK-ARM, bare-metal firmware

---

## Ownership Contract

配置与运行时操作的边界固定如下：

| Owner | Permitted responsibilities |
| --- | --- |
| `KG200Z_OPEN.ioc` / `Core/Src/*` | `Instance`、`Init`、GPIO 模式/复用、DMA、时钟、滤波器、NVIC 优先级及生成的 IRQ 入口 |
| `Usr/port/usr_port_platform.c` | 调用 `MX_GPIO_Init()`、`MX_USART1_UART_Init()`、`MX_SPI1_Init()`、`MX_SPI2_Init()`、`MX_I2C1_Init()`；执行板级恢复编排 |
| `Usr/port/usr_port_board.c` | 把 `huart1`、`hspi1`、`hspi2`、`hi2c1` 和物理引脚绑定到通用上下文 |
| `Usr/port/usr_port_hal_*.c` | 读取 MX 已生成的配置、校验需求、收发数据、维护 DMA 软件游标和回调 |
| `Usr/app` / `Usr/drivers/dev` | 声明设备需求与业务策略，不写 HAL 配置 |

允许的运行时调用包括 UART DMA 启停与收发、SPI 收发、I2C 事务、GPIO 读写和软件回调注册。禁止共享适配器调用 `HAL_UART_Init()`、`HAL_SPI_Init()`、`HAL_I2C_Init()`、`HAL_GPIO_Init()` 或 `HAL_NVIC_SetPriority()/EnableIRQ()`，也禁止写任何 `handle->Init` 字段。

## File Responsibility Map

- Modify: `KG200Z_OPEN.ioc` — 所有外设和 GPIO 的配置源。
- Regenerate: `Core/Src/spi.c`, `Core/Src/gpio.c`, `Core/Src/stm32wlxx_it.c`, `Core/Inc/main.h` — 只接受 CubeMX 生成结果。
- Modify: `Usr/port/usr_port_platform.c` — 按 MX 依赖顺序集中调用 GPIO、DMA 和业务外设初始化。
- Modify: `Usr/drivers/bus/usr_bus_uart.h`, `Usr/drivers/bus/usr_bus_uart.c` — UART 需求校验接口。
- Modify: `Usr/drivers/bus/usr_bus_spi.h`, `Usr/drivers/bus/usr_bus_spi.c` — SPI 需求校验接口和显式数据位宽。
- Modify: `Usr/drivers/bus/usr_bus_gpio.h`, `Usr/drivers/bus/usr_bus_gpio.c` — 删除运行时 GPIO 模式配置能力。
- Modify: `Usr/port/usr_port_hal.h` — I2C 重初始化回调和精简后的 GPIO 描述符。
- Modify: `Usr/port/usr_port_hal_uart.c`, `Usr/port/usr_port_hal_spi.c`, `Usr/port/usr_port_hal_i2c.c`, `Usr/port/usr_port_hal_gpio.c` — 无配置写入的 HAL 适配器。
- Modify: `Usr/port/usr_port.h`, `Usr/port/usr_port_platform.c`, `Usr/port/usr_port_board.c` — MX 调用、恢复回调和板级绑定。
- Modify: `Usr/drivers/dev/usr_dev_gps.c`, `Usr/app/usr_app.c`, `Usr/app/usr_app_config.h` — 声明并校验 GPS/SPI 需求。
- Modify: `Usr/DEVICE_DRIVER_FRAMEWORK.md`, `docs/superpowers/specs/2026-08-14-usr-port-reuse-design.md` — 更新所有权与使用说明。

<!-- END_SOURCE_FRAGMENT S001-F1 -->

## Source lines 123-188

<!-- SOURCE_FRAGMENT S001-F2 -->
### Task 2: Replace UART Mutation with Requirement Validation

**Files:**
- Modify: `Usr/drivers/bus/usr_bus_uart.h`
- Modify: `Usr/drivers/bus/usr_bus_uart.c`
- Modify: `Usr/port/usr_port_hal_uart.c`
- Modify: `Usr/drivers/dev/usr_dev_gps.c`

- [ ] **Step 1: Rename the bus contract from `set_cfg` to `validate_cfg`**

Use this interface in `usr_bus_uart.h`:

```c
typedef struct
{
  usr_status_t (*init)(void *ctx);
  usr_status_t (*deinit)(void *ctx);
  usr_status_t (*validate_cfg)(void *ctx, const usr_uart_cfg_t *cfg);
  usr_status_t (*write)(void *ctx, const uint8_t *data, uint16_t len,
                        uint32_t timeout_ms);
  usr_status_t (*read)(void *ctx, uint8_t *data, uint16_t capacity,
                       uint16_t *read_len);
  uint32_t     (*rx_overflow)(void *ctx);
} usr_uart_ops_t;

usr_status_t usr_bus_uart_validate_cfg(const usr_bus_uart_t *bus,
                                       const usr_uart_cfg_t *cfg);
```

Implement the bus forwarding function in `usr_bus_uart.c`:

```c
usr_status_t usr_bus_uart_validate_cfg(const usr_bus_uart_t *bus,
                                       const usr_uart_cfg_t *cfg)
{
  if ((bus == NULL) || (bus->backend.ops == NULL) ||
      (bus->backend.ops->validate_cfg == NULL) || (cfg == NULL))
  {
    return USR_ERR_PARAM;
  }
  return bus->backend.ops->validate_cfg(bus->backend.ctx, cfg);
}
```

- [ ] **Step 2: Make the HAL adapter read-only for UART configuration**

Replace the current UART `set_cfg` function with:

```c
static usr_status_t usr_port_hal_uart_validate_cfg(
    void *ctx, const usr_uart_cfg_t *cfg)
{
  const usr_port_hal_uart_t *uart = (const usr_port_hal_uart_t *)ctx;

  if (!usr_port_hal_uart_valid(uart) || (cfg == NULL) ||
      (cfg->baudrate == 0u))
  {
    return USR_ERR_PARAM;
  }
  return (uart->handle->Init.BaudRate == cfg->baudrate) ?
      USR_OK : USR_ERR_UNSUPPORTED;
}
```

Wire `usr_port_hal_uart_validate_cfg` into `usr_port_hal_uart_ops`. Do not stop DMA, deinitialize UART, write `BaudRate`, or call `HAL_UART_Init()` in this path. `HAL_UART_Receive_DMA()`, `HAL_UART_DMAStop()`, transmit, and read cursor maintenance remain runtime operations.

<!-- END_SOURCE_FRAGMENT S001-F2 -->

## Source lines 218-323

<!-- SOURCE_FRAGMENT S001-F3 -->
### Task 3: Replace SPI Reconfiguration with Complete Capability Validation

**Files:**
- Modify: `Usr/drivers/bus/usr_bus_spi.h`
- Modify: `Usr/drivers/bus/usr_bus_spi.c`
- Modify: `Usr/port/usr_port_hal_spi.c`
- Modify: `Usr/app/usr_app_config.h`
- Modify: `Usr/app/usr_app.c`

- [ ] **Step 1: Make SPI requirement semantics explicit**

Replace the device configuration and operation entry with:

```c
typedef struct
{
  uint32_t max_speed_hz; /* MX clock must not exceed this device limit */
  uint8_t  mode;         /* 0..3 = SPI MODE0..MODE3 */
  uint8_t  bit_order;    /* 0 = MSB first, 1 = LSB first */
  uint8_t  data_bits;    /* byte-buffer API currently requires 8 */
} usr_spi_dev_cfg_t;

typedef struct
{
  usr_status_t (*init)(void *ctx);
  usr_status_t (*validate_cfg)(void *ctx,
                               const usr_spi_dev_cfg_t *cfg);
  usr_status_t (*transmit)(void *ctx, const uint8_t *tx, uint8_t *rx,
                           uint16_t len, uint32_t timeout_ms);
} usr_spi_ops_t;

usr_status_t usr_bus_spi_validate_cfg(const usr_bus_spi_t *bus,
                                      const usr_spi_dev_cfg_t *cfg);
```

Implement `usr_bus_spi_validate_cfg()` with the same null/operation checks used by UART, then forward to `backend.ops->validate_cfg`.

- [ ] **Step 2: Replace prescaler selection with actual-setting inspection**

Delete `usr_port_hal_spi_prescaler()`. Add a read-only divisor helper:

```c
static uint16_t usr_port_hal_spi_divisor(uint32_t prescaler)
{
  switch (prescaler)
  {
    case SPI_BAUDRATEPRESCALER_2:   return 2u;
    case SPI_BAUDRATEPRESCALER_4:   return 4u;
    case SPI_BAUDRATEPRESCALER_8:   return 8u;
    case SPI_BAUDRATEPRESCALER_16:  return 16u;
    case SPI_BAUDRATEPRESCALER_32:  return 32u;
    case SPI_BAUDRATEPRESCALER_64:  return 64u;
    case SPI_BAUDRATEPRESCALER_128: return 128u;
    case SPI_BAUDRATEPRESCALER_256: return 256u;
    default:                        return 0u;
  }
}
```

Replace the mutating SPI function with:

```c
static usr_status_t usr_port_hal_spi_validate_cfg(
    void *ctx, const usr_spi_dev_cfg_t *cfg)
{
  const usr_port_hal_spi_t *spi = (const usr_port_hal_spi_t *)ctx;
  uint16_t divisor;
  uint32_t actual_speed_hz;
  uint32_t expected_phase;
  uint32_t expected_polarity;
  uint32_t expected_first_bit;

  if ((spi == NULL) || (spi->handle == NULL) ||
      (spi->kernel_clock_hz == 0u) || (cfg == NULL) ||
      (cfg->max_speed_hz == 0u) || (cfg->mode > 3u) ||
      (cfg->bit_order > 1u) || (cfg->data_bits != 8u))
  {
    return USR_ERR_PARAM;
  }
  divisor = usr_port_hal_spi_divisor(
      spi->handle->Init.BaudRatePrescaler);
  if (divisor == 0u)
  {
    return USR_ERR_UNSUPPORTED;
  }
  actual_speed_hz = spi->kernel_clock_hz / divisor;
  expected_phase = (cfg->mode & 1u) ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
  expected_polarity = (cfg->mode & 2u) ?
      SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
  expected_first_bit = (cfg->bit_order != 0u) ?
      SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;

  if ((actual_speed_hz > cfg->max_speed_hz) ||
      (spi->handle->Init.CLKPhase != expected_phase) ||
      (spi->handle->Init.CLKPolarity != expected_polarity) ||
      (spi->handle->Init.FirstBit != expected_first_bit) ||
      (spi->handle->Init.DataSize != SPI_DATASIZE_8BIT))
  {
    return USR_ERR_UNSUPPORTED;
  }
  return USR_OK;
}
```

Wire this into `usr_port_hal_spi_ops`. The file must no longer write any SPI `Init` member or call `HAL_SPI_Init()`.

<!-- END_SOURCE_FRAGMENT S001-F3 -->

## Source lines 363-446

<!-- SOURCE_FRAGMENT S001-F4 -->
### Task 4: Delegate I2C Recovery to the Complete MX Initialization Path

**Files:**
- Modify: `Usr/port/usr_port_hal.h`
- Modify: `Usr/port/usr_port_hal_i2c.c`
- Modify: `Usr/port/usr_port.h`
- Modify: `Usr/port/usr_port_platform.c`
- Modify: `Usr/port/usr_port_board.c`

- [ ] **Step 1: Inject a board-owned reinitialization callback**

Define the callback and extend the I2C context in `usr_port_hal.h`:

```c
typedef usr_status_t (*usr_port_hal_reinit_fn_t)(void *ctx);

typedef struct
{
  I2C_HandleTypeDef *handle;
  usr_port_hal_reinit_fn_t reinit;
  void *reinit_ctx;
} usr_port_hal_i2c_t;
```

Declare the KG200Z implementation in `usr_port.h`:

```c
usr_status_t usr_port_platform_sensor_i2c_reinit(void *ctx);
```

- [ ] **Step 2: Put the full recovery sequence at the MX boundary**

Implement in `usr_port_platform.c`:

```c
usr_status_t usr_port_platform_sensor_i2c_reinit(void *ctx)
{
  I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)ctx;

  if (handle != &hi2c1)
  {
    return USR_ERR_PARAM;
  }
  if (HAL_I2C_DeInit(handle) != HAL_OK)
  {
    return USR_ERR_BUS;
  }
  MX_I2C1_Init();
  return (HAL_I2C_GetState(handle) == HAL_I2C_STATE_READY) ?
      USR_OK : USR_ERR_BUS;
}
```

`HAL_I2C_DeInit()` only resets lifecycle state; no configuration value is reproduced here. `MX_I2C1_Init()` remains the only reconfiguration path and therefore restores timing, addressing, MSP resources, and both MX-generated filters together.

- [ ] **Step 3: Make the generic I2C adapter delegate only**

Replace `usr_port_hal_i2c_recover()` with:

```c
static usr_status_t usr_port_hal_i2c_recover(void *ctx)
{
  usr_port_hal_i2c_t *i2c = (usr_port_hal_i2c_t *)ctx;

  if ((i2c == NULL) || (i2c->handle == NULL) ||
      (i2c->reinit == NULL))
  {
    return USR_ERR_UNSUPPORTED;
  }
  return i2c->reinit(i2c->reinit_ctx);
}
```

Bind it in `usr_port_board.c`:

```c
static usr_port_hal_i2c_t s_sensor_i2c =
{
  .handle = &hi2c1,
  .reinit = usr_port_platform_sensor_i2c_reinit,
  .reinit_ctx = &hi2c1,
};
```

<!-- END_SOURCE_FRAGMENT S001-F4 -->

## Source lines 462-490

<!-- SOURCE_FRAGMENT S001-F5 -->
### Task 5: Remove the Remaining GPIO and NVIC Configuration Mutators

This task is required for the ownership claim: software CS and ICP EXTI currently still call `HAL_GPIO_Init()` and `HAL_NVIC_*()` from `Usr/port`.

**Files:**
- Modify: `Usr/drivers/bus/usr_bus_gpio.h`
- Modify: `Usr/drivers/bus/usr_bus_gpio.c`
- Modify: `Usr/port/usr_port_hal.h`
- Modify: `Usr/port/usr_port_hal_gpio.c`
- Modify: `Usr/port/usr_port_board.c`
- Modify: `Usr/app/usr_app.c`

- [ ] **Step 1: Delete runtime mode configuration from the generic GPIO API**

Remove `usr_gpio_mode_t`, the `set_mode` operation, and `usr_bus_gpio_set_mode()`. The resulting operation table is:

```c
typedef struct
{
  usr_status_t (*init)(void *ctx);
  usr_status_t (*write)(void *ctx, usr_gpio_pin_id_t pin, uint8_t level);
  usr_status_t (*read)(void *ctx, usr_gpio_pin_id_t pin, uint8_t *level);
  usr_status_t (*set_irq)(void *ctx, usr_gpio_pin_id_t pin,
                          usr_callback_t cb, void *arg);
} usr_gpio_ops_t;
```

Remove the corresponding forwarding function from `usr_bus_gpio.c`.

<!-- END_SOURCE_FRAGMENT S001-F5 -->
