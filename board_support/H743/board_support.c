#include "usr_port.h"
#include "usr_port_hal.h"
#include "main.h"
#include "usart.h"
#include "board_support.h"
#include "port_hal_uart.h"
#include "port_hal_i2c.h"
#include "port_hal_spi.h"
#include "port_hal_tim.h"
#include "port_hal_clock.h"
#include "port_hal_dma.h"
#include "port_hal_pwm.h"

#include "FreeRTOS.h"
#include "task.h"

extern const dev_gpio_ops_t port_hal_gpio_ops;

#define UART_RING_BUF_SIZE 256u
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))



PORT_HAL_DEFINE_CLOCK_FUNCTIONS(
    port_hal_spi4, RCC_PERIPHCLK_SPI4, Spi45ClockSelection,
    RCC_SPI45CLKSOURCE_PCLK2, SPI, 4, GPIO, E)
// PORT_HAL_DEFINE_CLOCK_FUNCTIONS_NO_KERNEL(
//     port_hal_spi1, SPI, 1, GPIO, A)
// PORT_HAL_DEFINE_CLOCK_FUNCTIONS(
//     port_hal_spi2, RCC_PERIPHCLK_I2S2, I2s2ClockSelection,
//     RCC_I2S2CLKSOURCE_HSI, SPI, 2, GPIO, B)
// PORT_HAL_DEFINE_CLOCK_FUNCTIONS(
//     port_hal_i2c1, RCC_PERIPHCLK_I2C1, I2c1ClockSelection,
//     RCC_I2C1CLKSOURCE_PCLK1, I2C, 1, GPIO, B)


/************************************************************************/
/*************************GPIO RESOURCE***********************************/
/************************************************************************/
static usr_callback_t s_gpio_irq_callbacks[USR_PIN_COUNT];
static void *s_gpio_irq_args[USR_PIN_COUNT];

PORT_HAL_DEFINE_GPIO_CLOCK_ENABLE_FUNCTION(gpiob, GPIO, B)
PORT_HAL_DEFINE_GPIO_CLOCK_ENABLE_FUNCTION(gpioe, GPIO, E)

static const usr_port_hal_gpio_pin_t s_gpio_pins[USR_PIN_COUNT] = {
    [USR_PIN_WDOG] =
        {
            .port = WDI_GPIO_Port,
            .pin = WDI_Pin,
            .clock_enable = gpiob_clock_enable,
            .mode = GPIO_MODE_OUTPUT_PP,
            .pull = GPIO_NOPULL,
            .speed = GPIO_SPEED_FREQ_LOW,
            .initial_level = GPIO_PIN_SET,
            .write_initial = true,
        },
    [USR_PIN_FRAM_CS] =
        {
            .port = FRAM_CS_GPIO_Port,
            .pin = FRAM_CS_Pin,
            .clock_enable = gpioe_clock_enable,
            .mode = GPIO_MODE_OUTPUT_PP,
            .pull = GPIO_NOPULL,
            .speed = GPIO_SPEED_FREQ_LOW,
            .initial_level = GPIO_PIN_SET,
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
static uint32_t usr_port_spi4_kernel_clock_hz(void)
{
  return HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI4);//获取SPI4时钟源时钟频率
}


PORT_HAL_DEFINE_CLOCK_FUNCTIONS(port_hal_uart1, RCC_PERIPHCLK_USART1,
                                Usart16ClockSelection,
                                RCC_USART16CLKSOURCE_PCLK2, USART, 1, GPIO, A)

PORT_HAL_DEFINE_CLOCK_FUNCTIONS(port_hal_uart2, RCC_PERIPHCLK_USART2,
                                Usart234578ClockSelection,
                                RCC_USART2CLKSOURCE_D2PCLK1, USART, 2, GPIO, A)

PORT_HAL_DEFINE_CLOCK_FUNCTIONS(port_hal_uart3, RCC_PERIPHCLK_USART3,
                                Usart234578ClockSelection,
                                RCC_USART3CLKSOURCE_D2PCLK1, USART, 3, GPIO, B)


static DMA_HandleTypeDef s_usart1_rx_dma;
static uint8_t s_uart_ring_storage[3][UART_RING_BUF_SIZE];

static const usr_port_hal_uart_resource_t s_uart_resources[UART_INDEX_MAX] = {
    [UART_INDEX_0] =
        {
            .instance = USART1,
            .gpio_port = GPIOA,
            .gpio_pins = GPIO_PIN_9 | GPIO_PIN_10,
            .gpio_alternate = GPIO_AF7_USART1,
            .irq = USART1_IRQn,
            .irq_priority = 0u,

            .rx_dma =
                {
                    .handle = &s_usart1_rx_dma,
                    .instance = DMA1_Stream0,
                    .init =
                        {
                            .Direction = DMA_PERIPH_TO_MEMORY,
                            .PeriphInc = DMA_PINC_DISABLE,
                            .MemInc = DMA_MINC_ENABLE,
                            .PeriphDataAlignment = DMA_PDATAALIGN_BYTE,
                            .MemDataAlignment = DMA_MDATAALIGN_BYTE,
                            .Mode = DMA_CIRCULAR,
                            .Priority = DMA_PRIORITY_LOW,
                        },
                    .irq = DMA1_Stream0_IRQn,
                    .irq_priority = 0u,
                    .irq_subpriority = 0u,
                    .clock_enable = port_hal_dma_controller_clock_enable,
                },

            .clock_config = port_hal_uart1_clock_config,
            .clock_enable = port_hal_uart1_clock_enable,
            .clock_disable = port_hal_uart1_clock_disable,
            .gpio_clock_enable = port_hal_uart1_gpio_clock_enable,
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
  },
    [UART_INDEX_2] = {
        .instance = USART3,
        .gpio_port = GPIOB,
        .gpio_pins = GPIO_PIN_10 | GPIO_PIN_11,
        .gpio_alternate = GPIO_AF7_USART3,
        .irq = USART3_IRQn,
        .irq_priority = 2u,

        .rx_dma = {0},
        
    .clock_config = port_hal_uart3_clock_config,
    .clock_enable = port_hal_uart3_clock_enable,
    .clock_disable = port_hal_uart3_clock_disable,
    .gpio_clock_enable = port_hal_uart3_gpio_clock_enable,
  },
  // [UART_INDEX_2] = {
  //   .instance = USART6,
  //   .gpio_port = GPIOG,
  //   .gpio_pins = GPIO_PIN_14 | GPIO_PIN_9,
  //   .gpio_alternate = GPIO_AF7_USART6,
  //   .irq = USART6_IRQn,
  //   .irq_priority = 2u,
  //   .rx_dma = {0},
  //   // .clock_config = port_hal_lpuart1_clock_config,
  //   // .clock_enable = port_hal_lpuart1_clock_enable,
  //   // .clock_disable = port_hal_lpuart1_clock_disable,
  //   // .gpio_clock_enable = port_hal_lpuart1_gpio_clock_enable,
  // },
};

static usr_port_hal_uart_instance_t s_uart_instances[UART_INDEX_MAX];
/************************************************************************/
/*************************UART RESOURCE***********************************/
/************************************************************************/


/************************************************************************/
/*************************TIM RESOURCE***********************************/
/************************************************************************/

PORT_HAL_DEFINE_PERIPHERAL_CLOCK_ENABLE_FUNCTION(port_hal_tim2, TIM, 2);
PORT_HAL_DEFINE_PERIPHERAL_CLOCK_DISABLE_FUNCTION(port_hal_tim2, TIM, 2);

PORT_HAL_DEFINE_PERIPHERAL_CLOCK_ENABLE_FUNCTION(port_hal_pwm5, TIM, 5);
PORT_HAL_DEFINE_PERIPHERAL_CLOCK_DISABLE_FUNCTION(port_hal_pwm5, TIM, 5);

PORT_HAL_DEFINE_GPIO_CLOCK_ENABLE_FUNCTION(gpioh, GPIO, H);


static const usr_port_hal_tim_resource_t
    s_timer_resources[TIM_INDEX_MAX] = {
  [TIM_INDEX_0] = {
    .instance = TIM2,
    .timer_clock_hz = 24000000u,
    .prescaler = (1000000 / 1000u) - 1,
    .auto_reload = 240 - 1,
    .clock_enable = port_hal_tim2_clock_enable,
    .clock_disable = port_hal_tim2_clock_disable,
    .has_update_irq = true,
    .update_irq = TIM2_IRQn,
    .irq_priority = 2u,
  },
};

static usr_port_hal_tim_instance_t s_timer_instances[TIM_INDEX_MAX] = {
    [TIM_INDEX_0] =
        {
            .resource = &s_timer_resources[TIM_INDEX_0],
            .handle = &htim2,
            .device =
                {
                    .ctx = &s_timer_instances[TIM_INDEX_0],
                    .ops = &port_hal_tim_ops,
                },
        },
};

const usr_port_hal_pwm_channel_t s_tim5_pwm_channels[] = {
    [DEV_PWM_CHANNEL_1] =
        {
            .logical_channel = DEV_PWM_CHANNEL_1,
            .hal_channel = TIM_CHANNEL_1,
            .compare = 500000,
            .polarity = TIM_OCPOLARITY_HIGH,

            .gpio_port = PWMOUT2_GPIO_Port,
            .gpio_pin = PWMOUT2_Pin,
            .gpio_alternate = GPIO_AF2_TIM5,
            .gpio_clock_enable = gpioh_clock_enable,
        },
    [DEV_PWM_CHANNEL_2] =
        {
            .logical_channel = DEV_PWM_CHANNEL_2,
            .hal_channel = TIM_CHANNEL_2,
            .compare = 500000,
            .polarity = TIM_OCPOLARITY_HIGH,

            .gpio_port = PWMOUT3_GPIO_Port,
            .gpio_pin = PWMOUT3_Pin,
            .gpio_alternate = GPIO_AF2_TIM5,
            .gpio_clock_enable = gpioh_clock_enable,
        },
    [DEV_PWM_CHANNEL_3] =
        {
            .logical_channel = DEV_PWM_CHANNEL_3,
            .hal_channel = TIM_CHANNEL_3,
            .compare = 500000,
            .polarity = TIM_OCPOLARITY_HIGH,

            .gpio_port = PWMOUT4_GPIO_Port,
            .gpio_pin = PWMOUT4_Pin,
            .gpio_alternate = GPIO_AF2_TIM5,
            .gpio_clock_enable = gpioh_clock_enable,
        },
};

static const usr_port_hal_pwm_resource_t s_pwm_resources[PWM_INDEX_MAX] = {
    [PWM_INDEX_0] = {
        .instance = TIM5,
        .timer_clock_hz = 24000000u,
        .prescaler = 240 - 1,
        .auto_reload = 1000000 - 1,
        .clock_enable = port_hal_pwm5_clock_enable,
        .clock_disable = port_hal_pwm5_clock_disable,



        .has_update_irq = true,
        .update_irq = TIM5_IRQn,
        .irq_priority = 2u,

        .pwm_channels = s_tim5_pwm_channels,
        .pwm_channel_count = sizeof(s_tim5_pwm_channels) / sizeof(s_tim5_pwm_channels[0]),
  },
};

static usr_port_hal_pwm_instance_t s_pwm_instances[PWM_INDEX_MAX] = {
  [PWM_INDEX_0] = {
    .resource = &s_pwm_resources[PWM_INDEX_0],
    .handle = &htim5,
    .device = {
      .ctx = &s_pwm_instances[PWM_INDEX_0],
      .ops = &port_hal_pwm_ops,
    },
  },
};
/************************************************************************/
/*************************TIM RESOURCE***********************************/
/************************************************************************/




/************************************************************************/
/*************************SPI RESOURCE***********************************/
/************************************************************************/
static const usr_port_hal_spi_resource_t s_spi_resources[SPI_INDEX_MAX] = {
    [SPI_INDEX_0] = {
        .instance = SPI4,
        .gpio_port = GPIOE,
        .gpio_pins = GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_6,
        .gpio_alternate = GPIO_AF5_SPI4,
        .rx_dma = {0},
        .enable_irq = false,
        .clock_config = port_hal_spi4_clock_config,
        .clock_enable = port_hal_spi4_clock_enable,
        .clock_disable = port_hal_spi4_clock_disable,
        .gpio_clock_enable = port_hal_spi4_gpio_clock_enable,
        .kernel_clock_hz = usr_port_spi4_kernel_clock_hz,
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
/*************************SPI RESOURCE***********************************/
/************************************************************************/

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
    // .clock_config = port_hal_i2c1_clock_config,
    // .clock_enable = port_hal_i2c1_clock_enable,
    // .clock_disable = port_hal_i2c1_clock_disable,
    // .gpio_clock_enable = port_hal_i2c1_gpio_clock_enable,
  },
};

static usr_port_hal_i2c_instance_t s_i2c_instances[I2C_INDEX_MAX] = {
    [I2C_INDEX_0] =
        {
            .resource = &s_i2c_resources[I2C_INDEX_0],
            .device =
                {
                    .ctx = &s_i2c_instances[I2C_INDEX_0],
                    .ops = &port_hal_i2c_ops,
                },
        },
};
/************************************************************************/
/*************************IIC RESOURCE***********************************/
/************************************************************************/

_Static_assert(ARRAY_SIZE(s_uart_resources) == ARRAY_SIZE(s_uart_instances),
               "UART resource and instance counts must match");
_Static_assert(ARRAY_SIZE(s_spi_resources) == ARRAY_SIZE(s_spi_instances),
               "SPI resource and instance counts must match");
_Static_assert(ARRAY_SIZE(s_i2c_resources) == ARRAY_SIZE(s_i2c_instances),
               "I2C resource and instance counts must match");


/************************************************************************/
/*************************TIMER RESOURCE***********************************/
/************************************************************************/
static usr_port_hal_timer_t s_timer_ctx = {
    // .handle = &htim2,
};

static timer_device_t s_timer_device = {
    .ctx = &s_timer_ctx,
    .ops = &port_hal_tim_ops,
};
/************************************************************************/
/*************************TIMER RESOURCE***********************************/
/************************************************************************/



static uint32_t usr_port_board_get_ms() {
#if _RTOS
  return xTaskGetTickCount();
#else
   return HAL_GetTick();
#endif
}

static void usr_port_board_delay_ms(uint32_t ms) {
#if _RTOS
  vTaskDelay(ms);
#else
  HAL_Delay(ms);
#endif
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
    if (s_uart_instances[index].handle == handle)
    {
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

usr_port_hal_tim_instance_t *usr_port_board_tim_find_instance(
                    const TIM_HandleTypeDef *handle) {
  uint32_t index;

  if (handle == NULL)
  {
    return NULL;
  }
  for (index = 0u; index < ARRAY_SIZE(s_spi_instances); index++)
  {
    if (s_timer_instances[index].handle == handle)
    {
      return &s_timer_instances[index];
    }
  }
  return NULL;
}

usr_port_hal_pwm_instance_t *usr_port_board_pwm_find_instance(
                    const TIM_HandleTypeDef *handle) {
  uint32_t index;

  if (handle == NULL)
  {
    return NULL;
  }
  for (index = 0u; index < ARRAY_SIZE(s_spi_instances); index++)
  {
    if (s_pwm_instances[index].handle == handle)
    {
      return &s_pwm_instances[index];
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

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  usr_port_hal_gpio_irq_dispatch(&s_gpio_ctx, GPIO_Pin);
}


gpio_device_t *get_gpio_device(void)
{
  return &s_gpio_device;
}

// uart_device_t *get_uart_device(uint8_t index)
// {
//   if (index >= UART_INDEX_MAX)
//   {
//     return NULL;
//   }
//   return &s_uart_instances[index].device;
// }

time_dev_t *get_time_device(void)
{
  return &s_time_device;
}

// spi_device_t *get_spi_device(uint8_t index)
// {
//   if (index >= SPI_INDEX_MAX)
//   {
//     return NULL;
//   }
//   return &s_spi_instances[index].device;
// }

i2c_device_t *get_i2c_device(void)
{
  return &s_i2c_instances[I2C_INDEX_0].device;
}

timer_device_t *get_timer_device(void) { return &s_timer_device; }

timer_device_t *get_tim_device(uint8_t index) {
  if (index >= TIM_INDEX_MAX)
  {
    return NULL;
  }
  return &s_timer_instances[index].device;
}

pwm_device_t *get_pwm_device(uint8_t index) {
  if (index >= TIM_INDEX_MAX)
  {
    return NULL;
  }
  return &s_pwm_instances[index].device;
}
