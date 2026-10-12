# Historical framework guide excerpts

> Authority: historical source excerpt, not current API or current verification.
> Source: S009, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `Usr/DEVICE_DRIVER_FRAMEWORK.md`.

Framework, bus, port and migration sections only. Device protocol and facade sections excluded; old usage examples are not current APIs.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-63

<!-- SOURCE_FRAGMENT S009-F1 -->
# 设备驱动框架说明与使用指南

## 1. 框架定位

`Usr` 采用分层架构和依赖注入设计，将设备协议、通用总线和 STM32 HAL 解耦。设备驱动只依赖抽象总线接口；更换 UART、I2C、SPI 实例或更换 MCU 时，通常只需要修改 `Usr/port` 的适配和板级绑定。

```text
main.c
  -> Usr/app       应用组合根：创建、注入和调度
  -> Usr/device    面向业务的设备门面
  -> Usr/drivers   设备协议和通用总线
  -> Usr/port      STM32 HAL 适配器与板卡资源绑定
  -> STM32 HAL
```

分层约束如下：

- `Usr/common`：状态码、时间接口、回调类型等公共定义。
- `Usr/drivers/bus`：GPIO、UART、I2C、SPI 的平台无关接口和通用策略。
- `Usr/drivers/dev`：GPS、ICP 等设备协议、状态机和数据解析。
- `Usr/device`：给应用层使用的稳定门面 API。
- `Usr/port`：`ops + ctx` 后端、STM32 HAL 调用、具体外设句柄、引脚、IRQ 和时钟。
- `Usr/app`：组合对象、配置策略、初始化顺序和主循环调度。

HAL 类型不得进入 `Usr/drivers` 或 `Usr/device`。

## 2. 硬件配置所有权

硬件配置按外设逐步迁移，不再把所有外设统一绑定到生成的 `MX_*_Init()`：

- SPI1/SPI2 已由 `Usr/port/port_bsp/port_hal_spi.c` 完整持有。该文件私有保存 HAL handle、外设实例、内核时钟配置、外设和 GPIO 时钟门控、GPIO AF 以及默认 SPI 参数；`port_hal_spi_init()` 只准备 handle 并调用 `HAL_SPI_Init()`。
- `HAL_SPI_Init()` 按 HAL 约定回调 Core 中的 `HAL_SPI_MspInit()`；Core 只负责调用 `usr_port_hal_spi_owns()` 并把 port-owned handle 分发到 `usr_port_hal_spi_msp_init()`。具体 SPI 时钟、GPIO AF 和 SPI2 的 `PB13/PB14/PB15` 配置仍由 `Usr/port/port_bsp/port_hal_spi.c` 完成。
- `usr_port_hal_spi_owns()` 只接受 `port_hal_spi.c` 内部静态 context 的私有 handle 地址；即使外部 handle 使用相同的 SPI instance，也不会获得这些 GPIO、时钟或 deinit 资源。
- `usr_port_platform_init()` 不再调用 `MX_SPI1_Init()` 或 `MX_SPI2_Init()`；Core 中不再保存或导出 SPI handle，也不再实现 SPI1/SPI2 的具体资源配置，仅保留 HAL MSP dispatcher。
- GPIO、I2C 和保留的 Core 服务仍按各自当前实现初始化；SPI 迁移不改变它们的所有权。
- SPI 软件片选继续作为逻辑 GPIO 资源初始化和控制，不放入 SPI controller context。

SPI port 的默认参数是 Master、2-line full duplex、8-bit、MODE0、MSB first、software NSS 和 prescaler 16。SPI1 使用 `PA5=SCK`、`PA6=MISO`、`PA7=MOSI` 及 `GPIO_AF5_SPI1`；SPI2 使用 `PB13=SCK`、`PB14=MISO`、`PB15=MOSI` 及 `GPIO_AF5_SPI2`。PB13 当前专用于 SPI2 SCK，不再作为外部 ADC 输入；ADC 外部输入只保留 PB2。

## 3. `ops + ctx` 后端模型

每种总线由两部分组成：

- `ops`：该类外设支持的操作函数表，例如读写、初始化、恢复。
- `ctx`：某一个具体实例的运行状态和私有硬件资源，例如 HAL handle、资源描述、DMA 缓冲区、时钟和 GPIO 描述表。

SPI1 和 SPI2 共享 `port_hal_spi_ops`，但各自使用 `port_hal_spi.c` 中静态分配的独立 context。`Usr/port/board_support.c` 只通过 `usr_port_hal_spi_get_context()` 获取 opaque context 并装配 `spi_device_t`，不接触或传入 HAL handle。

SPI 对上层暴露的 ops 保持为：

```c
typedef struct
{
  usr_status_t (*init)(void *ctx);
  usr_status_t (*deinit)(void *ctx);
  usr_status_t (*transmit)(void *ctx,
                           const uint8_t *tx, uint16_t tx_len,
                           uint8_t *rx, uint16_t rx_len,
                           uint32_t timeout_ms);
} dev_spi_ops_t;
```

HAL SPI context 和物理资源结构只在 [port_hal_spi.c](port/port_bsp/port_hal_spi.c) 内可见；公开声明见 [port_hal_spi.h](port/port_bsp/port_hal_spi.h)，板级装配见 [board_support.c](port/board_support.c) 和 [board_support.h](port/board_support.h)。
<!-- END_SOURCE_FRAGMENT S009-F1 -->

## Source lines 78-150

<!-- SOURCE_FRAGMENT S009-F2 -->
## 5. 总线使用说明

### GPIO

应用和设备只使用 `USR_PIN_*` 逻辑编号。逻辑编号到物理端口、引脚和 IRQ 的映射由 port 层完成。

```c
usr_bus_gpio_set_irq(&gpio, USR_PIN_ICP_INT, callback, arg);
```

引脚模式和 EXTI/NVIC 已由 CubeMX 初始化，运行时不重复配置。

### UART

UART 对上层提供协议无关的字节流。STM32 HAL 适配器使用循环 DMA 接收，GPS 的 NMEA 分行和解析由 GPS 驱动完成。

```c
uint8_t data[32];
uint16_t length;
usr_bus_uart_read(&gps_uart, data, sizeof(data), &length);
```

### I2C

I2C 地址使用 7 位格式；寄存器地址长度支持 1 或 2 字节。bus 层负责默认超时、忙状态保护、故障标记、恢复和重试。

```c
usr_bus_i2c_read_reg(&i2c, address7, reg, 1u,
                     data, length, timeout_ms);
```

### SPI

当前 SPI 是 polling/blocking 实现。公共 `dev_spi_ops_t` 提供 `init()`、`deinit()` 和 `transmit()`；`transmit()` 根据 tx/rx 参数调用 HAL 的阻塞发送、接收或全双工接口，并把 HAL 状态映射为项目错误码。HAL 类型和调用不进入设备协议层。

SPI 初始化和反初始化沿用 UART 的 MSP 形式：

```text
dev_spi_init()
  -> port_hal_spi_init()
  -> HAL_SPI_Init()
  -> HAL_SPI_MspInit()
  -> usr_port_hal_spi_owns()
  -> usr_port_hal_spi_msp_init()
  -> SPI clock/GPIO AF setup

HAL_SPI_DeInit()
  -> HAL_SPI_MspDeInit()
  -> usr_port_hal_spi_msp_deinit()
```

`Core/Src/spi.c` 只保留 MSP dispatcher，不保存 SPI handle，也不实现
SPI1/SPI2 的实例、引脚或时钟分支。

SPI bus 对外使用以下真实接口：

```c
dev_spi_init(spi);
dev_spi_deinit(spi);
dev_spi_transact(spi, cs_pin, tx, tx_len, rx, rx_len, timeout_ms);
dev_spi_begin(spi, cs_pin);
dev_spi_frame(spi, tx, tx_len, rx, rx_len, timeout_ms);
dev_spi_end(spi, cs_pin);
```

`dev_spi_transact()` 完成一次“拉低 CS、传输、释放 CS”的事务。需要在多段传输间保持片选时，使用 `dev_spi_begin()`、一次或多次 `dev_spi_frame()`、最后 `dev_spi_end()`。CS 是 GPIO 逻辑资源：SPI1 FRAM 使用 `PA4 / USR_PIN_PRIMARY_SPI_CS`，SPI2 ICP 使用 `PB12 / USR_PIN_SECONDARY_SPI_CS`；SPI controller ops 不硬编码具体 CS。

SPI1/SPI2 当前不启用 DMA，没有为 SPI 分配 DMA channel、DMA handle 或 SPI DMA IRQ。将来如果启用 SPI DMA，DMA handle、channel/request、IRQ 分发、传输状态和 buffer lifetime 仍由 `Usr/port` 私有管理，并通过 SPI bus ops 承担异步时序；在没有实际共享需求前不新增公共 `dma_device_t`。

### 时间服务

设备通过 `usr_time_t` 获取毫秒计时和延时，不直接调用 `HAL_GetTick()` 或 `HAL_Delay()`，因此可以替换为其他 MCU 实现或测试时钟。

<!-- END_SOURCE_FRAGMENT S009-F2 -->

## Source lines 199-212

<!-- SOURCE_FRAGMENT S009-F3 -->
## 8. 更换实例或移植到其他 MCU

同一 STM32 HAL 平台更换 SPI 实例时，调整 `Usr/port/port_bsp/port_hal_spi.c` 中的：

- 外设 instance；
- SPI 内核时钟、外设时钟和 GPIO 时钟回调；
- SCK/MISO/MOSI 物理引脚和 AF；
- SPI 默认参数；
- 未来可能加入的 DMA 资源和 IRQ 分发。

片选引脚单独在 `Usr/port/board_support.c` 的逻辑 GPIO 表中绑定。设备协议继续只保存 `spi_device_t`、逻辑 CS 和芯片参数。

跨 MCU 时替换 `Usr/port`：重新实现 `dev_*_ops_t`，保持 `dev_spi_*`、设备驱动和应用配置接口不变。

<!-- END_SOURCE_FRAGMENT S009-F3 -->
