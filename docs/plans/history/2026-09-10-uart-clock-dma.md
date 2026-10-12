# Historical UART clock and DMA proposal

> Authority: historical source excerpt, not current API or current verification.
> Source: S003, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/plans/2026-09-10-uart-clock-dma-resource-decoupling.md`.

Historical UART/MSP/DMA contracts. Physical mappings and retained Core ownership apply only to the old host version.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-111

<!-- SOURCE_FRAGMENT S003-F1 -->
# UART Clock and DMA Resource Decoupling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove USART1-specific clock, GPIO clock, and DMA assumptions from the generic UART MSP path, restore `__HAL_LINKDMA()`, give USART1 RX a collision-free DMA channel, and make Core MSP port-first so adding another port-owned UART later is a resource record, not another Core `Instance==USARTx` branch.

**Architecture:** Keep the reusable `Usr/drivers` UART API unchanged. In `Usr/port`, each UART resource record owns kernel-clock selection, peripheral gate, GPIO port gate, pins, and optional RX DMA. `usr_port_hal_uart_owns()` is the Core MSP probe. `usr_port_hal_uart_msp_init()` / `usr_port_hal_uart_msp_deinit()` only run for owned records and invoke those callbacks. Move the USART1 RX DMA handle into port-private static storage, link it with `__HAL_LINKDMA()`, and route `DMA1_Channel2` to that handle. Keep ADC on `DMA1_Channel1`. Keep USART2/LPUART1 on the existing Core-generated path.

**Tech Stack:** STM32WLE5 HAL, C11 bare-metal firmware, static resource tables, CubeMX-generated Core services, `__HAL_LINKDMA()`, clangd MCP plus `rg` for ownership audits.

**Spec:** User request on 2026-09-10, continued on 2026-09-11: explain the UART clock configuration, remove the hardcoded MSP clock/gates, restore the standard UART/DMA link pattern, and produce one internally consistent implementation plan. This file is that plan. Do not implement firmware until the user chooses an execution option.

## Global Constraints

- `Usr/port` is the only user layer allowed to include STM32 HAL/LL/vendor peripheral headers or call MCU-specific APIs.
- Keep GPS USART1 ownership in `Usr/port`; keep trace paths through LPUART1/USART2 and RTT intact.
- Keep DMA private to the port adapter and board binding layer; do not introduce a public `dma_device_t`.
- This slice has one DMA map: ADC keeps `DMA1_Channel1`; port-owned USART1 RX moves to `DMA1_Channel2`; SPI1 DMA stays disabled. A later SPI DMA migration must choose a different channel before enabling it.
- Never invoke `HAL_DMA_IRQHandler()` twice for the same handle or the same DMA channel.
- A generic UART resource lookup must remain table-driven by `handle->Instance`. Adding USART2, LPUART1, or another supported UART later requires a new resource record and private callbacks, not another `if (instance == ...)` block in the generic MSP function and not another Core `Instance==USARTx` owner branch.
- Use `__HAL_LINKDMA(uartHandle, hdmarx, *resource->rx_dma_handle)` only after `HAL_DMA_Init()` succeeds; do not hand-maintain the equivalent `hdmarx`/`Parent` assignment in the UART MSP function.
- Do not close a GPIO port clock in UART deinit because GPIO ports may be shared; deinit may close only the UART peripheral gate and the DMA handle owned by that resource.
- Do not migrate Core-owned USART2/LPUART1 setup in this slice. Do not add USART2 or LPUART1 rows to `s_uart_resources[]`.
- `MX_DMA_Init()` remains Core global DMA clock/NVIC infrastructure. Do not move controller clock or channel NVIC setup into `Usr/port` in this slice.
- Do not claim the current `.ioc` owns USART1: `KG200Z_OPEN.ioc` currently has no USART1 peripheral/DMA entry. USART1 remains a port-owned source-level binding; LPUART1 remains CubeMX/Core-owned.
- Core MSP must become port-first. Put the ownership probe in USER CODE at the top of `HAL_UART_MspInit()` / `HAL_UART_MspDeInit()`. Delete the entire `Instance==USART1` dispatcher. CubeMX regen must not restore a USART1 owner branch because `.ioc` has no USART1.
- Unknown UART instances must return from port MSP. Incomplete owned records may call `Error_Handler()` in MSP init because `HAL_UART_MspInit()` has no status return.
- Preserve unrelated working-tree changes and do not add generated binaries, maps, object files, or IDE state.
- Use two-space indentation, C11, ASCII, and ST/`Usr` brace style for later code.
- Update `docs/change_log/CHANGELOG.md` with purpose, files, API impact, dependency impact, exact verification, risks, and hardware status.
- Do not commit unless the user explicitly authorizes a commit.

## Current Ownership And Clock Contract

The live GPS path is:

```text
usr_app_init()
  -> dev_uart_init(get_uart_device())
  -> port_hal_uart_init()
  -> HAL_UART_Init(&huart1)
  -> HAL_UART_MspInit()
  -> usr_port_hal_uart_msp_init()
```

`get_uart_device()` returns `&uart_device[0]`, which binds `&huart1` / USART1 / PA9/PA10. `MX_USART1_UART_Init()` is commented in `usr_port_platform.c`. Keep the definition. GPS owner is `dev_uart_init()`, not `MX_USART1_UART_Init()`.

`port_hal_uart_find_resource()` already matches `handle->Instance`, but the selected record is ignored for clock and gate operations. The generic MSP currently writes:

```c
PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
__HAL_RCC_USART1_CLK_ENABLE();
__HAL_RCC_GPIOA_CLK_ENABLE();
```

and deinit always calls `__HAL_RCC_USART1_CLK_DISABLE()`. That is the hardcoding under review: adding LPUART1 or USART2 must not mean another branch in this generic function.

STM32WLxx kernel clocks are not one generic integer. `RCC_PeriphCLKInitTypeDef` has separate fields:

| UART | Selection mask | Field | Current source | Gate | Pins |
| --- | --- | --- | --- | --- | --- |
| USART1 | `RCC_PERIPHCLK_USART1` | `Usart1ClockSelection` | `RCC_USART1CLKSOURCE_PCLK2` | `__HAL_RCC_USART1_CLK_*` | PA9/PA10, `GPIO_AF7_USART1` |
| USART2 | `RCC_PERIPHCLK_USART2` | `Usart2ClockSelection` | `RCC_USART2CLKSOURCE_PCLK1` | `__HAL_RCC_USART2_CLK_*` | PA2/PA3, `GPIO_AF7_USART2` |
| LPUART1 | `RCC_PERIPHCLK_LPUART1` | `Lpuart1ClockSelection` | `RCC_LPUART1CLKSOURCE_LSE` | `__HAL_RCC_LPUART1_CLK_*` | PB10/PB11, `GPIO_AF8_LPUART1` |

USART1 may also select SYSCLK, HSI, or LSE. GPS stays on PCLK2. Those vendor-specific fields belong in resource callbacks. The generic MSP only calls `resource->clock_config()`, `resource->clock_enable()`, `resource->gpio_clock_enable()`, and later `resource->clock_disable()`.

Current DMA/IRQ facts that this plan must not leave ambiguous:

- Port USART1 RX currently points at generated `hdma_usart1_rx` / `DMA1_Channel1`.
- Core ADC also uses `hdma_adc` / `DMA1_Channel1`. `SYS_GetBatteryLevel()` calls `MX_ADC_Init()`, and `ADC_ReadMultiChannels()` calls `HAL_ADC_Start_DMA()`. ADC is a live Core consumer, not a dormant leftover.
- `DMA1_Channel1_IRQHandler()` currently calls `HAL_DMA_IRQHandler(&hdma_usart1_rx)`, which is the wrong owner for ADC.
- SPI1 DMA init is commented out in `Core/Src/spi.c`. `hdma_spi1_rx` is defined there but not declared in `Core/Inc/spi.h`.
- `DMA1_Channel2_IRQHandler()` currently calls `HAL_DMA_IRQHandler(&hdma_spi1_rx)`. That leftover IRQ owner is what this slice replaces; this slice does not enable SPI DMA.
- `MX_DMA_Init()` already enables Channel1 and Channel2 NVIC.
- `Core/Src/usart_if.c` still contains `MX_USART1_UART_Init()` under `#ifdef USART_MODULE`. `Core/Inc/main.h` wraps `#define USART_MODULE` in `#if 0`, so that path is inactive. Treat `rg` as the authority for `#ifdef` and comments. Clangd `find_references` for `MX_USART1_UART_Init` currently returns the definition only.
- `HAL_UART_MspInit()` currently has an `Instance==USART1` branch that calls `usr_port_hal_uart_msp_init()` and returns; the generated USART1 MSP after that `return` is unreachable. This slice deletes that dispatcher instead of keeping it.
- `Core/Src/usart.c` also contains a second `HAL_UART_MspInit()` under `#ifdef USART_ENABLE`. That path is inactive. Do not treat it as the live MSP owner.

`__HAL_LINKDMA()` in `Drivers/STM32WLxx_HAL_Driver/Inc/stm32wlxx_hal_def.h` expands to:

```c
(__HANDLE__)->__PPP_DMA_FIELD__ = &(__DMA_HANDLE__);
(__DMA_HANDLE__).Parent = (__HANDLE__);
```

The current port MSP duplicates that by hand:

```c
uartHandle->hdmarx = resource->rx_dma_handle;
resource->rx_dma_handle->Parent = uartHandle;
```

Replace it with `__HAL_LINKDMA(uartHandle, hdmarx, *resource->rx_dma_handle)` after successful `HAL_DMA_Init()`.

Clangd is required in verification, but it is not proof of link success, DMA channel conflict, IRQ double-dispatch, `#ifdef` callers, or hardware GPS/ADC behavior. Rechecked on 2026-09-11, clangd:

- reports 0 errors on `port_hal_uart.c`, `usart.c`, and `stm32wlxx_it.c`
- finds `usr_port_hal_uart_msp_init` at `port_hal_uart.c:406`, callers at `usart.c:231` and `usr_port_hal.h:56`
- finds `usr_port_hal_uart_msp_deinit` at `port_hal_uart.c:455`, callers at `usart.c:367` and `usr_port_hal.h:57`
- finds `hdma_usart1_rx` at `usart.c:31`, including IRQ use at `stm32wlxx_it.c:185` and resource use at `port_hal_uart.c:28`
- finds `hdma_adc` at `adc.c:27`
- finds `hdma_spi1_rx` at `spi.c:37`, including IRQ use at `stm32wlxx_it.c:199`
- finds `MX_USART1_UART_Init` at `usart.c:80`; `find_references` returns the definition only
- does not find `usr_port_hal_uart_owns` or `usr_port_hal_uart_dma_irq` yet
- reports unused-variable warnings in `board_support.c` for `gps_device`/`icp_device`/`hcd_device`; those are unrelated and must not be "fixed" in this slice
- returns 0 incoming calls for `HAL_UART_MspInit`; do not treat empty incoming hierarchy as proof of no callers

Use clangd for types, missing declarations, duplicate definitions, and named-function call sites. Use `rg` as the authority for DMA/IRQ ownership and `#ifdef` callers. Use `scripts/build.ps1` as the authority for compile/link.

<!-- END_SOURCE_FRAGMENT S003-F1 -->

## Source lines 219-592

<!-- SOURCE_FRAGMENT S003-F2 -->
## Task 2: Add Ownership Probe, Resource-Specific Clock Callbacks, And Port-Owned USART1 DMA Storage

**Files:**
- Modify: `Usr/port/port_bsp/port_hal_uart.c`
- Modify: `Usr/port/usr_port_hal.h`
- Modify: `Usr/port/port_bsp/port_hal_uart.h` only if a private declaration is required by the board binding; the current header does not need a change for this task

**Interfaces:**
- Consumes: `UART_HandleTypeDef *` passed by HAL and the existing `usr_port_hal_uart_resource_t` table.
- Produces: an ownership probe, resource callbacks with these exact signatures, plus port-private USART1 RX DMA storage:

```c
bool usr_port_hal_uart_owns(const UART_HandleTypeDef *uartHandle);

typedef HAL_StatusTypeDef (*usr_port_hal_uart_clock_config_fn_t)(void);
typedef void (*usr_port_hal_uart_clock_gate_fn_t)(void);

typedef struct
{
  USART_TypeDef *instance;
  GPIO_TypeDef *gpio_port;
  uint16_t gpio_pins;
  uint32_t gpio_alternate;
  DMA_HandleTypeDef *rx_dma_handle;
  DMA_Channel_TypeDef *rx_dma_instance;
  uint32_t rx_dma_request;
  usr_port_hal_uart_clock_config_fn_t clock_config;
  usr_port_hal_uart_clock_gate_fn_t clock_enable;
  usr_port_hal_uart_clock_gate_fn_t clock_disable;
  usr_port_hal_uart_clock_gate_fn_t gpio_clock_enable;
} usr_port_hal_uart_resource_t;
```

- [ ] **Step 1: Declare the ownership probe**

In `usr_port_hal.h`, next to the existing MSP declarations, add:

```c
bool usr_port_hal_uart_owns(const UART_HandleTypeDef *uartHandle);
void usr_port_hal_uart_msp_init(UART_HandleTypeDef *uartHandle);
void usr_port_hal_uart_msp_deinit(UART_HandleTypeDef *uartHandle);
```

Leave `usr_port_hal_uart_dma_irq()` for Task 4. `bool` is already used in this header.

- [ ] **Step 2: Move USART1 RX DMA storage into the port file**

In `port_hal_uart.c`, remove the `usart.h` include if it exists only to reach `hdma_usart1_rx`. Keep `stm32wlxx_hal.h` and `port_hal_uart.h`. If `Error_Handler()` is no longer visible after dropping `usart.h`, include `main.h` in this file only; do not add a public `hdma_usart1_rx` replacement. Add:

```c
static DMA_HandleTypeDef s_usart1_rx_dma;
```

Do not add a public `extern DMA_HandleTypeDef hdma_usart1_rx` replacement. IRQ code in a later task must go through `usr_port_hal_uart_dma_irq()`.

- [ ] **Step 3: Implement the USART1 callback set**

Add four `static` functions in `port_hal_uart.c` using the current board values exactly:

```c
static HAL_StatusTypeDef port_hal_uart1_clock_config(void)
{
  RCC_PeriphCLKInitTypeDef config = {0};

  config.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  config.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  return HAL_RCCEx_PeriphCLKConfig(&config);
}

static void port_hal_uart1_clock_enable(void)
{
  __HAL_RCC_USART1_CLK_ENABLE();
}

static void port_hal_uart1_clock_disable(void)
{
  __HAL_RCC_USART1_CLK_DISABLE();
}

static void port_hal_uart1_gpio_clock_enable(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
}
```

- [ ] **Step 4: Attach callbacks and Channel2 DMA fields to the USART1 resource**

Replace the current table entry with:

```c
static const usr_port_hal_uart_resource_t s_uart_resources[] =
{
  {
    .instance = USART1,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO_PIN_10 | GPIO_PIN_9,
    .gpio_alternate = GPIO_AF7_USART1,
    .rx_dma_handle = &s_usart1_rx_dma,
    .rx_dma_instance = DMA1_Channel2,
    .rx_dma_request = DMA_REQUEST_USART1_RX,
    .clock_config = port_hal_uart1_clock_config,
    .clock_enable = port_hal_uart1_clock_enable,
    .clock_disable = port_hal_uart1_clock_disable,
    .gpio_clock_enable = port_hal_uart1_gpio_clock_enable,
  },
};
```

Do not keep `&hdma_usart1_rx` or `DMA1_Channel1` in this record. Do not add USART2 or LPUART1 rows.

- [ ] **Step 5: Add complete-resource validation and the ownership probe**

```c
static bool port_hal_uart_resource_valid(
    const usr_port_hal_uart_resource_t *resource)
{
  if (resource == NULL)
  {
    return false;
  }
  if ((resource->clock_config == NULL) ||
      (resource->clock_enable == NULL) ||
      (resource->clock_disable == NULL) ||
      (resource->gpio_clock_enable == NULL) ||
      (resource->gpio_port == NULL) ||
      (resource->gpio_pins == 0u) ||
      (resource->instance == NULL))
  {
    return false;
  }
  if ((resource->rx_dma_handle != NULL) &&
      (resource->rx_dma_instance == NULL))
  {
    return false;
  }
  return true;
}

bool usr_port_hal_uart_owns(const UART_HandleTypeDef *uartHandle)
{
  return (port_hal_uart_find_resource(uartHandle) != NULL);
}
```

`usr_port_hal_uart_owns()` returns false for unknown instances, including current Core-owned LPUART1 and USART2. Incomplete owned records still fail `port_hal_uart_resource_valid()` and may `Error_Handler()` in MSP init. Unknown instances must not.

- [ ] **Step 6: Document future UART records, but do not add them**

Keep these examples in the changelog later, not in `s_uart_resources[]`. Adding them now would create two owners for Core-owned handles.

USART2 example fields:

```c
config.PeriphClockSelection = RCC_PERIPHCLK_USART2;
config.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
__HAL_RCC_USART2_CLK_ENABLE();
```

LPUART1 example fields:

```c
config.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
config.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_LSE;
__HAL_RCC_LPUART1_CLK_ENABLE();
```

## Task 3: Make MSP Init/Deinit Dispatch Through The Selected Resource

**Files:**
- Modify: `Usr/port/port_bsp/port_hal_uart.c`

**Interfaces:**
- Consumes: `usr_port_hal_uart_owns()`, `port_hal_uart_resource_valid()`, and the callback-bearing `usr_port_hal_uart_resource_t` from Task 2.
- Produces: `usr_port_hal_uart_msp_init()` and `usr_port_hal_uart_msp_deinit()` with no direct USART1/GPIOA clock operations.

- [ ] **Step 1: Replace hardcoded clock setup in MSP init**

Replace the current clock block in `usr_port_hal_uart_msp_init()` with:

```c
void usr_port_hal_uart_msp_init(UART_HandleTypeDef *uartHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  const usr_port_hal_uart_resource_t *resource;

  if (!usr_port_hal_uart_owns(uartHandle))
  {
    return;
  }

  resource = port_hal_uart_find_resource(uartHandle);
  if (!port_hal_uart_resource_valid(resource))
  {
    Error_Handler();
    return;
  }

  if (resource->clock_config() != HAL_OK)
  {
    Error_Handler();
    return;
  }
  resource->clock_enable();
  resource->gpio_clock_enable();

  GPIO_InitStruct.Pin = resource->gpio_pins;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = resource->gpio_alternate;
  HAL_GPIO_Init(resource->gpio_port, &GPIO_InitStruct);

  if ((resource->rx_dma_handle != NULL) && (resource->rx_dma_instance != NULL))
  {
    resource->rx_dma_handle->Instance = resource->rx_dma_instance;
    resource->rx_dma_handle->Init.Request = resource->rx_dma_request;
    resource->rx_dma_handle->Init.Direction = DMA_PERIPH_TO_MEMORY;
    resource->rx_dma_handle->Init.PeriphInc = DMA_PINC_DISABLE;
    resource->rx_dma_handle->Init.MemInc = DMA_MINC_ENABLE;
    resource->rx_dma_handle->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    resource->rx_dma_handle->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    resource->rx_dma_handle->Init.Mode = DMA_CIRCULAR;
    resource->rx_dma_handle->Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(resource->rx_dma_handle) != HAL_OK)
    {
      Error_Handler();
      return;
    }
    __HAL_LINKDMA(uartHandle, hdmarx, *resource->rx_dma_handle);
  }
}
```

Unknown UART instances return. Incomplete owned records call `Error_Handler()`. After port-first Core MSP, LPUART1 and USART2 must take the unknown-UART return, not `Error_Handler()`.

The generic function must not contain `RCC_PERIPHCLK_USART1`, `Usart1ClockSelection`, `RCC_USART1CLKSOURCE_PCLK2`, `__HAL_RCC_USART1_CLK_ENABLE()`, `__HAL_RCC_GPIOA_CLK_ENABLE()`, `uartHandle->hdmarx =`, or `Parent =` after this task.

- [ ] **Step 2: Replace hardcoded clock setup in MSP deinit**

```c
void usr_port_hal_uart_msp_deinit(UART_HandleTypeDef *uartHandle)
{
  const usr_port_hal_uart_resource_t *resource;

  if (!usr_port_hal_uart_owns(uartHandle))
  {
    return;
  }

  resource = port_hal_uart_find_resource(uartHandle);
  if (!port_hal_uart_resource_valid(resource))
  {
    return;
  }

  HAL_GPIO_DeInit(resource->gpio_port, resource->gpio_pins);
  if (uartHandle->hdmarx != NULL)
  {
    HAL_DMA_DeInit(uartHandle->hdmarx);
    uartHandle->hdmarx = NULL;
  }
  resource->clock_disable();
}
```

Do not reverse `HAL_RCCEx_PeriphCLKConfig()`. Do not disable the GPIO port clock. Unknown UART instances return.

- [ ] **Step 3: Scan for generic MSP leakage**

Run:

```powershell
rg -n "RCC_PERIPHCLK_USART1|Usart1ClockSelection|RCC_USART1CLKSOURCE_PCLK2|__HAL_RCC_USART1_CLK_(ENABLE|DISABLE)|__HAL_RCC_GPIOA_CLK_ENABLE|hdmarx =|Parent =" Usr/port/port_bsp/port_hal_uart.c
```

Expected: concrete USART1/GPIOA clock names remain only inside the four `port_hal_uart1_*` callbacks. `__HAL_LINKDMA` is the only UART/DMA association in MSP init.

## Task 4: Export The USART1 DMA IRQ Wrapper And Retarget Channel Owners

**Files:**
- Modify: `Usr/port/port_bsp/port_hal_uart.c`
- Modify: `Usr/port/usr_port_hal.h`
- Modify: `Core/Src/stm32wlxx_it.c`
- Audit: `Core/Src/spi.c`, `Core/Inc/spi.h`, `Core/Src/adc.c`, `Core/Src/dma.c`

**Interfaces:**
- Consumes: Task 2 static DMA handle `s_usart1_rx_dma` and Task 3 linked `uartHandle->hdmarx`.
- Produces:

```c
void usr_port_hal_uart_dma_irq(UART_HandleTypeDef *uartHandle);
```

and a Core IRQ map of Channel1 = ADC, Channel2 = port USART1 RX.

- [ ] **Step 1: Add the port DMA IRQ wrapper**

In `usr_port_hal.h`, next to the existing MSP declarations, the complete set after this task is:

```c
bool usr_port_hal_uart_owns(const UART_HandleTypeDef *uartHandle);
void usr_port_hal_uart_msp_init(UART_HandleTypeDef *uartHandle);
void usr_port_hal_uart_msp_deinit(UART_HandleTypeDef *uartHandle);
void usr_port_hal_uart_dma_irq(UART_HandleTypeDef *uartHandle);
```

In `port_hal_uart.c` add:

```c
void usr_port_hal_uart_dma_irq(UART_HandleTypeDef *uartHandle)
{
  const usr_port_hal_uart_resource_t *resource;

  resource = port_hal_uart_find_resource(uartHandle);
  if ((resource == NULL) || (resource->rx_dma_handle == NULL))
  {
    return;
  }
  HAL_DMA_IRQHandler(resource->rx_dma_handle);
}
```

Call `HAL_DMA_IRQHandler()` exactly once per IRQ. Do not also call it from the Core handler for the same handle.

- [ ] **Step 2: Retarget Core DMA IRQs**

In `Core/Src/stm32wlxx_it.c`:

1. Include `usr_port_hal.h` and `adc.h`.
2. Remove `extern DMA_HandleTypeDef hdma_usart1_rx;`.
3. Keep `extern UART_HandleTypeDef huart1;` if it is not already visible through an included header.
4. Replace the Channel1 and Channel2 bodies with:

```c
void DMA1_Channel1_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_adc);
}

void DMA1_Channel2_IRQHandler(void)
{
  usr_port_hal_uart_dma_irq(&huart1);
}
```

Do not leave `HAL_DMA_IRQHandler(&hdma_spi1_rx)` on Channel2. Do not leave `HAL_DMA_IRQHandler(&hdma_usart1_rx)` on Channel1. Do not add a second `HAL_DMA_IRQHandler()` in either handler.

- [ ] **Step 3: Confirm leftover SPI DMA is not enabled**

Run:

```powershell
rg -n "hdma_spi1_rx|DMA1_Channel2|__HAL_LINKDMA\(spiHandle" Core/Src/spi.c Core/Inc/spi.h Core/Src/stm32wlxx_it.c
```

Expected after this task:

- `spi.c` still has commented SPI1 DMA init
- `spi.h` still has no `hdma_spi1_rx` declaration
- `stm32wlxx_it.c` Channel2 no longer names `hdma_spi1_rx`
- `MX_DMA_Init()` still enables Channel2 NVIC

Do not uncomment SPI DMA. If a later SPI DMA slice needs a channel, it must not reuse Channel1 or Channel2.

- [ ] **Step 4: Confirm ADC remains on Channel1**

Run:

```powershell
rg -n "hdma_adc|DMA1_Channel1|HAL_ADC_Start_DMA|MX_ADC_Init" Core/Src/adc.c Core/Src/adc_if.c Core/Src/stm32wlxx_it.c
```

Expected: ADC handle and request stay on `DMA1_Channel1`; IRQ now dispatches `hdma_adc`; no ADC source change is required.

<!-- END_SOURCE_FRAGMENT S003-F2 -->

## Source lines 879-892

<!-- SOURCE_FRAGMENT S003-F3 -->
## Self-Review Checklist

- Adding LPUART1 or USART2 to this adapter requires a new resource record and callbacks; it does not require adding another concrete instance branch to `usr_port_hal_uart_msp_init()` and it does not require another Core `Instance==USARTx` owner branch.
- Clock-source selection is not represented as one generic integer because STM32 HAL uses separate fields for USART1, USART2, and LPUART1.
- GPIO port clocks are enabled per resource but intentionally not disabled in deinit because ports are shared.
- `__HAL_LINKDMA()` is restored only for the UART-owned DMA handle; the public driver still sees no HAL or DMA type.
- DMA controller clock/NVIC setup and per-channel HAL handle setup stay separate. `MX_DMA_Init()` remains Core infrastructure.
- This slice does not offer DMA schemes A/B/C. Channel1 is ADC, Channel2 is port USART1 RX, SPI1 DMA stays disabled.
- `hdma_usart1_rx` is deleted from Core after the port static handle exists. IRQ code never dual-dispatches that handle.
- `MX_USART1_UART_Init()` is retained but is not the GPS owner.
- Core MSP is port-first. Unknown UART instances return from port MSP. Incomplete owned records may `Error_Handler()` in MSP init.
- Clangd is required and insufficient: `rg` covers `#ifdef` and DMA/IRQ ownership; `scripts/build.ps1` covers compile/link; hardware remains a separate gate.
- No public `Usr/drivers` API is widened in this slice.

<!-- END_SOURCE_FRAGMENT S003-F3 -->
