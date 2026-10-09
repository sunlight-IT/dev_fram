#include "usr_port.h"
#include "usr_port_hal.h"
#include "main.h"
#include "usart.h"
#include "board_support.h"
#include "port_hal_uart.h"
#include "port_hal_i2c.h"
#include "port_hal_spi.h"
#include "port_hal_tim.h"
#include "port_hal_pwm.h"
#include "port_hal_clock.h"
#include "port_hal_dma.h"
extern const dev_gpio_ops_t port_hal_gpio_ops;

#define UART_RING_BUF_SIZE 256u
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

PORT_HAL_DEFINE_CLOCK_FUNCTIONS(
    port_hal_uart1, RCC_PERIPHCLK_USART1, Usart1ClockSelection,
    RCC_USART1CLKSOURCE_PCLK2, USART, 1, GPIO, A)
PORT_HAL_DEFINE_CLOCK_FUNCTIONS(
    port_hal_uart2, RCC_PERIPHCLK_USART2, Usart2ClockSelection,
    RCC_USART2CLKSOURCE_PCLK1, USART, 2, GPIO, A)
PORT_HAL_DEFINE_CLOCK_FUNCTIONS(
    port_hal_lpuart1, RCC_PERIPHCLK_LPUART1, Lpuart1ClockSelection,
    RCC_LPUART1CLKSOURCE_LSE, LPUART, 1, GPIO, B)
PORT_HAL_DEFINE_CLOCK_FUNCTIONS_NO_KERNEL(
    port_hal_spi1, SPI, 1, GPIO, A)
PORT_HAL_DEFINE_CLOCK_FUNCTIONS_NO_KERNEL(
    port_hal_spi2, SPI, 2, GPIO, B)
PORT_HAL_DEFINE_CLOCK_FUNCTIONS(
    port_hal_i2c1, RCC_PERIPHCLK_I2C1, I2c1ClockSelection,
    RCC_I2C1CLKSOURCE_PCLK1, I2C, 1, GPIO, B)


/************************************************************************/
/*************************GPIO RESOURCE***********************************/
/************************************************************************/
static usr_callback_t s_gpio_irq_callbacks[USR_PIN_COUNT];
static void *s_gpio_irq_args[USR_PIN_COUNT];

static void usr_port_gpioa_clock_enable(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

static void usr_port_gpiob_clock_enable(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

static const usr_port_hal_gpio_pin_t s_gpio_pins[USR_PIN_COUNT] = {
  [USR_PIN_GNSS_PW_EN] =
  {
    .port = GNSS_PW_EN_GPIO_Port,
    .pin = GNSS_PW_EN_Pin,
    .clock_enable = usr_port_gpioa_clock_enable,
    .mode = GPIO_MODE_OUTPUT_PP,
    .pull = GPIO_PULLDOWN,
    .speed = GPIO_SPEED_FREQ_LOW,
    .initial_level = GPIO_PIN_RESET,
    .write_initial = true,
  },
  [USR_PIN_GNSS_RST] =
  {
    .port = GNSS_RST_GPIO_Port,
    .pin = GNSS_RST_Pin,
    .clock_enable = usr_port_gpioa_clock_enable,
    .mode = GPIO_MODE_OUTPUT_PP,
    .pull = GPIO_PULLUP,
    .speed = GPIO_SPEED_FREQ_LOW,
    .initial_level = GPIO_PIN_RESET,
    .write_initial = true,
  },
  [USR_PIN_GNSS_STANDBY] =
  {
    .port = GNSS_STANDBY_GPIO_Port,
    .pin = GNSS_STANDBY_Pin,
    .clock_enable = usr_port_gpioa_clock_enable,
    .mode = GPIO_MODE_OUTPUT_PP,
    .pull = GPIO_PULLDOWN,
    .speed = GPIO_SPEED_FREQ_LOW,
    .initial_level = GPIO_PIN_RESET,
    .write_initial = true,
  },
  [USR_PIN_ICP_INT] =
  {
    .port = ICP20_INT_GPIO_Port,
    .pin = ICP20_INT_Pin,
    .clock_enable = usr_port_gpiob_clock_enable,
    .mode = GPIO_MODE_IT_FALLING,
    .pull = GPIO_PULLUP,
    .speed = GPIO_SPEED_FREQ_LOW,
    .has_irq = true,
    .irq = EXTI1_IRQn,
    .irq_priority = 3u,
    .irq_subpriority = 0u,
  },
  [USR_PIN_PRIMARY_SPI_CS] =
  {
    .port = PRIMARY_SPI_CS_GPIO_Port,
    .pin = PRIMARY_SPI_CS_Pin,
    .clock_enable = usr_port_gpioa_clock_enable,
    .mode = GPIO_MODE_OUTPUT_PP,
    .pull = GPIO_NOPULL,
    .speed = GPIO_SPEED_FREQ_LOW,
    .initial_level = GPIO_PIN_SET,
    .write_initial = true,
  },
  [USR_PIN_SECONDARY_SPI_CS] =
  {
    .port = SECONDARY_SPI_CS_GPIO_Port,
    .pin = SECONDARY_SPI_CS_Pin,
    .clock_enable = usr_port_gpiob_clock_enable,
    .mode = GPIO_MODE_OUTPUT_PP,
    .pull = GPIO_NOPULL,
    .speed = GPIO_SPEED_FREQ_LOW,
    .initial_level = GPIO_PIN_SET,
    .write_initial = true,
  },
  [USR_PIN_WDOG] =
  {
    .port = WDI_GPIO_Port,
    .pin = WDI_Pin,
    .clock_enable = usr_port_gpioa_clock_enable,
    .mode = GPIO_MODE_OUTPUT_PP,
    .pull = GPIO_NOPULL,
    .speed = GPIO_SPEED_FREQ_LOW,
    .initial_level = GPIO_PIN_RESET,
    .write_initial = true,
  },
};

static usr_port_hal_gpio_t s_gpio_ctx = {
    .pins = s_gpio_pins,
    .irq_callbacks = s_gpio_irq_callbacks,
    .irq_args = s_gpio_irq_args,
    .pin_count = USR_PIN_COUNT,
};

static gpio_device_t s_gpio_device = {
    .ctx = &s_gpio_ctx,
    .ops = &port_hal_gpio_ops,
};
/************************************************************************/
/*************************GPIO RESOURCE***********************************/
/************************************************************************/



/************************************************************************/
/*************************UART RESOURCE***********************************/
/************************************************************************/
static uint32_t usr_port_spi1_kernel_clock_hz(void)
{
  return HAL_RCC_GetPCLK2Freq();
}

static uint32_t usr_port_spi2_kernel_clock_hz(void)
{
  return HAL_RCC_GetPCLK1Freq();
}

static uint32_t usr_port_tim2_kernel_clock_hz(void)
{
  RCC_ClkInitTypeDef clock_config;
  uint32_t flash_latency;
  uint32_t pclk;

  pclk = HAL_RCC_GetPCLK1Freq();
  HAL_RCC_GetClockConfig(&clock_config, &flash_latency);
  if (clock_config.APB1CLKDivider == RCC_HCLK_DIV1)
  {
    return pclk;
  }
  return pclk * 2u;
}

static void usr_port_tim2_clock_enable(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();
}

static void usr_port_tim2_clock_disable(void)
{
  __HAL_RCC_TIM2_CLK_DISABLE();
}

static DMA_HandleTypeDef s_usart1_rx_dma;
static uint8_t s_uart_ring_storage[UART_INDEX_MAX][UART_RING_BUF_SIZE];

static const usr_port_hal_uart_resource_t s_uart_resources[UART_INDEX_MAX] = {
  [UART_INDEX_0] = {
    .instance = USART1,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO_PIN_10 | GPIO_PIN_9,
    .gpio_alternate = GPIO_AF7_USART1,
    .irq = USART1_IRQn,
    .irq_priority = 0u,
    .rx_dma = {
      .handle = &s_usart1_rx_dma,
      .instance = DMA1_Channel2,
      .init = {
        .Request = DMA_REQUEST_USART1_RX,
        .Direction = DMA_PERIPH_TO_MEMORY,
        .PeriphInc = DMA_PINC_DISABLE,
        .MemInc = DMA_MINC_ENABLE,
        .PeriphDataAlignment = DMA_PDATAALIGN_BYTE,
        .MemDataAlignment = DMA_MDATAALIGN_BYTE,
        .Mode = DMA_CIRCULAR,
        .Priority = DMA_PRIORITY_LOW,
      },
      .irq = DMA1_Channel2_IRQn,
      .irq_priority = 0u,
      .irq_subpriority = 0u,
      .clock_enable = port_hal_dma_controller_clock_enable,
    },
    .clock_config = port_hal_uart1_clock_config,
    .clock_enable = port_hal_uart1_clock_enable,
    .clock_disable = port_hal_uart1_clock_disable,
    .gpio_clock_enable = port_hal_uart1_gpio_clock_enable,
    .default_config = UART1_CONFIG_DEFAULT,
  },
  [UART_INDEX_1] = {
    .instance = USART2,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO_PIN_2 | GPIO_PIN_3,
    .gpio_alternate = GPIO_AF7_USART2,
    .irq = USART2_IRQn,
    .irq_priority = 2u,
    .rx_dma = {0},
    .clock_config = port_hal_uart2_clock_config,
    .clock_enable = port_hal_uart2_clock_enable,
    .clock_disable = port_hal_uart2_clock_disable,
    .gpio_clock_enable = port_hal_uart2_gpio_clock_enable,
    .default_config = UART2_CONFIG_DEFAULT,
  },
  [UART_INDEX_2] = {
    .instance = LPUART1,
    .gpio_port = GPIOB,
    .gpio_pins = GPIO_PIN_10 | GPIO_PIN_11,
    .gpio_alternate = GPIO_AF8_LPUART1,
    .irq = LPUART1_IRQn,
    .irq_priority = 2u,
    .rx_dma = {0},
    .clock_config = port_hal_lpuart1_clock_config,
    .clock_enable = port_hal_lpuart1_clock_enable,
    .clock_disable = port_hal_lpuart1_clock_disable,
    .gpio_clock_enable = port_hal_lpuart1_gpio_clock_enable,
    .default_config = LPUART1_CONFIG_DEFAULT,
  },
};

static usr_port_hal_uart_instance_t s_uart_instances[UART_INDEX_MAX];
/************************************************************************/
/*************************UART RESOURCE***********************************/
/************************************************************************/







/************************************************************************/
/*************************SPI RESOURCE***********************************/
/************************************************************************/
static const usr_port_hal_spi_resource_t s_spi_resources[SPI_INDEX_MAX] = {
  [SPI_INDEX_0] = {
    .instance = SPI1,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
    .gpio_alternate = GPIO_AF5_SPI1,
    .rx_dma = {0},
    .enable_irq = false,
    .clock_config = port_hal_spi1_clock_config,
    .clock_enable = port_hal_spi1_clock_enable,
    .clock_disable = port_hal_spi1_clock_disable,
    .gpio_clock_enable = port_hal_spi1_gpio_clock_enable,
    .kernel_clock_hz = usr_port_spi1_kernel_clock_hz,
    .default_config = {
      .max_speed_hz = 3000000u,
      .mode = DEV_SPI_MODE_0,
      .bit_order = DEV_SPI_BIT_ORDER_MSB_FIRST,
      .data_width = DEV_SPI_DATA_BITS_8,
    },
  },
  [SPI_INDEX_1] = {
    .instance = SPI2,
    .gpio_port = GPIOB,
    .gpio_pins = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
    .gpio_alternate = GPIO_AF5_SPI2,
    .rx_dma = {0},
    .enable_irq = false,
    .clock_config = port_hal_spi2_clock_config,
    .clock_enable = port_hal_spi2_clock_enable,
    .clock_disable = port_hal_spi2_clock_disable,
    .gpio_clock_enable = port_hal_spi2_gpio_clock_enable,
    .kernel_clock_hz = usr_port_spi2_kernel_clock_hz,
    .default_config = {
      .max_speed_hz = 3000000u,
      .mode = DEV_SPI_MODE_0,
      .bit_order = DEV_SPI_BIT_ORDER_MSB_FIRST,
      .data_width = DEV_SPI_DATA_BITS_8,
    },
  },
};

static usr_port_hal_spi_instance_t s_spi_instances[SPI_INDEX_MAX];

/************************************************************************/
/*************************IIC RESOURCE***********************************/
/************************************************************************/
static const usr_port_hal_i2c_resource_t s_i2c_resources[I2C_INDEX_MAX] = {
  [I2C_INDEX_0] = {
    .instance = I2C1,
    .gpio_port = GPIOB,
    .gpio_pins = GPIO_PIN_6 | GPIO_PIN_7,
    .gpio_alternate = GPIO_AF4_I2C1,
    .timing = 0x20309DEAu,
    .rx_dma = {0},
    .clock_config = port_hal_i2c1_clock_config,
    .clock_enable = port_hal_i2c1_clock_enable,
    .clock_disable = port_hal_i2c1_clock_disable,
    .gpio_clock_enable = port_hal_i2c1_gpio_clock_enable,
  },
};

static usr_port_hal_i2c_instance_t s_i2c_instances[I2C_INDEX_MAX] = {
  [I2C_INDEX_0] = {
    .resource = &s_i2c_resources[I2C_INDEX_0],
    .device = {
      .ctx = &s_i2c_instances[I2C_INDEX_0],
      .ops = &port_hal_i2c_ops,
    },
  },
};

_Static_assert(ARRAY_SIZE(s_uart_resources) == ARRAY_SIZE(s_uart_instances),
               "UART resource and instance counts must match");
_Static_assert(ARRAY_SIZE(s_spi_resources) == ARRAY_SIZE(s_spi_instances),
               "SPI resource and instance counts must match");
_Static_assert(ARRAY_SIZE(s_i2c_resources) == ARRAY_SIZE(s_i2c_instances),
               "I2C resource and instance counts must match");


/************************************************************************/
/*************************TIMER RESOURCE***********************************/
/************************************************************************/
static const usr_port_hal_tim_resource_t
    s_timer_resources[TIMER_INDEX_MAX] = {
  [TIMER_INDEX_SCHEDULER] = {
    .instance = TIM2,
    .counter_bits = 32u,
    .capability_mask = PORT_TIM_CAP_BASIC,
    .owner = USR_PORT_TIM_OWNER_SCHEDULER,
    .reserved_role = USR_PORT_TIM_ROLE_SCHEDULER_TICK,
    .conflict_domain = 2u,
    .clock_get_hz = usr_port_tim2_kernel_clock_hz,
    .clock_enable = usr_port_tim2_clock_enable,
    .clock_disable = usr_port_tim2_clock_disable,
    .has_update_irq = true,
    .has_capture_irq = false,
    .update_irq = TIM2_IRQn,
    .capture_irq = TIM2_IRQn,
    .irq_priority = 2u,
    .irq_subpriority = 0u,
    .channel_mask = 0u,
    .capture_channel = 0u,
    .gpio_port = NULL,
    .gpio_pin = 0u,
    .gpio_alternate = 0u,
    .gpio_clock_enable = NULL,
    .default_config = {
      .mode = DEV_TIMER_MODE_BASIC,
      .period_us = 1000u,
      .capture = {0},
    },
  },
};

static usr_port_hal_tim_instance_t s_timer_instances[TIMER_INDEX_MAX] = {
  [TIMER_INDEX_SCHEDULER] = {
    .resource = &s_timer_resources[TIMER_INDEX_SCHEDULER],
    .handle = &htim2,
    .device = {
      .ctx = &s_timer_instances[TIMER_INDEX_SCHEDULER],
      .config = {
        .mode = DEV_TIMER_MODE_BASIC,
        .period_us = 1000u,
        .capture = {0},
      },
      .ops = &port_hal_tim_ops,
    },
  },
};

static const uint32_t s_capture_resource_count = 0u;
static const uint32_t s_pwm_resource_count = 0u;
/************************************************************************/
/*************************TIMER RESOURCE***********************************/
/************************************************************************/



static uint32_t usr_port_board_get_ms()
{
  return HAL_GetTick();
}

static void usr_port_board_delay_ms(uint32_t ms)
{
  HAL_Delay(ms);
}

static const usr_time_ops_t s_time_ops =
{
  usr_port_board_get_ms,
  usr_port_board_delay_ms,
};

time_dev_t s_time_device = {
    .ops = &s_time_ops,
};

static usr_status_t usr_dev_uart_init(usr_port_hal_uart_instance_t *instance,uint32_t index) {

  if (instance == NULL || index >= UART_INDEX_MAX) {
    return USR_ERR_PARAM;
  }
  instance->device = get_uart_device(index);
  if (instance->device == NULL) {
    return USR_ERR_PARAM;
  }

  instance->device->ctx = instance;
  instance->device->ops = &port_hal_uart_ops;



  return USR_OK;
}
static usr_status_t usr_dev_spi_init(usr_port_hal_spi_instance_t *instance, gpio_device_t *gpio_device,
                                     uint32_t index) {
  if (instance == NULL || index >= SPI_INDEX_MAX) {
    return USR_ERR_PARAM;
  }
  instance->device = get_spi_device(index);
  if (instance->device == NULL) {
    return USR_ERR_PARAM;
  }

  instance->device->ctx = instance;
  instance->device->ops = &port_hal_spi_ops;
  instance->device->gpio = gpio_device;



  return USR_OK;
}

static void usr_instance_uart_init(usr_port_hal_uart_instance_t *instance, usr_port_hal_uart_resource_t *resource) {

  for (uint32_t index = 0u; index < UART_INDEX_MAX; index++) {
    usr_dev_uart_init(&instance[index], index);
    instance[index].resource = &resource[index];
    instance[index].handle = get_uart_table_handle(index);
    instance[index].ring.buffer = s_uart_ring_storage[index];
    instance[index].ring.capacity = UART_RING_BUF_SIZE;
  }
}

static void usr_instance_spi_init(usr_port_hal_spi_instance_t *instance,
                                  usr_port_hal_spi_resource_t *resource) {
  for (uint32_t index = 0u; index < SPI_INDEX_MAX; index++) {
    usr_dev_spi_init(&instance[index], get_gpio_device(), index);
    instance[index].resource = &resource[index];
    instance[index].handle = get_spi_table_handle(index);
    // instance[index].ring.buffer = s_spi_ring_storage[index];
    // instance[index].ring.capacity = SPI_RING_BUF_SIZE;
  }
}

    
usr_status_t usr_port_board_init(void)
{
  usr_status_t status;
  
  s_gpio_device.ctx = &s_gpio_ctx;
  status = s_gpio_device.ops->init(s_gpio_device.ctx);
  if (status != USR_OK)
  {
    return status;
  }



 usr_instance_uart_init(s_uart_instances, s_uart_resources);  
 usr_instance_spi_init(s_spi_instances, s_spi_resources);  


  (void)s_capture_resource_count;
  (void)s_pwm_resource_count;



  return USR_OK;
}

usr_port_hal_uart_instance_t *usr_port_board_uart_find_instance(
    const UART_HandleTypeDef *handle)
{
  uint32_t index;

  if (handle == NULL)
  {
    return NULL;
  }
  for (index = 0u; index < ARRAY_SIZE(s_uart_instances); index++)
  {
    if (s_uart_instances[index].handle == handle) {
      
      return &s_uart_instances[index];
    }
  }
  return NULL;
}

usr_port_hal_uart_instance_t *usr_port_board_uart_find_dma_instance(
    IRQn_Type irq)
{
  uint32_t index;

  for (index = 0u; index < ARRAY_SIZE(s_uart_instances); index++)
  {
    if (port_hal_dma_enabled(&s_uart_resources[index].rx_dma) &&
        (s_uart_resources[index].rx_dma.irq == irq))
    {
      return &s_uart_instances[index];
    }
  }
  return NULL;
}

usr_port_hal_spi_instance_t *usr_port_board_spi_find_instance(
    const SPI_HandleTypeDef *handle)
{
  uint32_t index;

  if (handle == NULL)
  {
    return NULL;
  }
  for (index = 0u; index < ARRAY_SIZE(s_spi_instances); index++)
  {
    if (s_spi_instances[index].handle == handle)
    {
      return &s_spi_instances[index];
    }
  }
  return NULL;
}

usr_port_hal_spi_instance_t *usr_port_board_spi_find_dma_instance(
    IRQn_Type irq)
{
  uint32_t index;

  for (index = 0u; index < ARRAY_SIZE(s_spi_instances); index++)
  {
    if (port_hal_dma_enabled(&s_spi_resources[index].rx_dma) &&
        (s_spi_resources[index].rx_dma.irq == irq))
    {
      return &s_spi_instances[index];
    }
  }
  return NULL;
}

usr_port_hal_i2c_instance_t *usr_port_board_i2c_find_instance(
    const I2C_HandleTypeDef *handle)
{
  uint32_t index;

  if (handle == NULL)
  {
    return NULL;
  }
  for (index = 0u; index < ARRAY_SIZE(s_i2c_instances); index++)
  {
    if (&s_i2c_instances[index].handle == handle)
    {
      return &s_i2c_instances[index];
    }
  }
  return NULL;
}

usr_port_hal_i2c_instance_t *usr_port_board_i2c_find_dma_instance(
    IRQn_Type irq)
{
  uint32_t index;

  for (index = 0u; index < ARRAY_SIZE(s_i2c_instances); index++)
  {
    if (port_hal_dma_enabled(&s_i2c_resources[index].rx_dma) &&
        (s_i2c_resources[index].rx_dma.irq == irq))
    {
      return &s_i2c_instances[index];
    }
  }
  return NULL;
}

usr_port_hal_tim_instance_t *usr_port_board_tim_find_instance(
    const TIM_HandleTypeDef *handle)
{
  uint32_t index;

  if (handle == NULL)
  {
    return NULL;
  }
  for (index = 0u; index < ARRAY_SIZE(s_timer_instances); index++)
  {
    if (s_timer_instances[index].handle == handle)
    {
      return &s_timer_instances[index];
    }
  }
  return NULL;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  usr_port_hal_gpio_irq_dispatch(&s_gpio_ctx, GPIO_Pin);
}


gpio_device_t *get_gpio_device(void)
{
  return &s_gpio_device;
}

time_dev_t *get_time_device(void)
{
  return &s_time_device;
}




i2c_device_t *get_i2c_device(void)
{
  return &s_i2c_instances[I2C_INDEX_0].device;
}

timer_device_t *get_timer_device(void)
{
  return &s_timer_instances[TIMER_INDEX_SCHEDULER].device;
}

pwm_device_t *get_pwm_device(uint8_t index)
{
  /* The current board has no verified PWM row: USR_ERR_UNSUPPORTED. */
  (void)index;
  return NULL;
}
