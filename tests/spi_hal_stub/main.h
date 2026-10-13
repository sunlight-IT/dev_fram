#ifndef SPI_TEST_MAIN_H
#define SPI_TEST_MAIN_H
#include "board_support.h"
extern uint32_t spi_test_primask;
extern uint32_t spi_test_ipsr;
extern uint32_t spi_test_basepri;
extern uint32_t spi_test_faultmask;
void spi_test_set_primask(uint32_t value);
#define __get_PRIMASK() (spi_test_primask)
#define __get_IPSR() (spi_test_ipsr)
#define __get_BASEPRI() (spi_test_basepri)
#define __get_FAULTMASK() (spi_test_faultmask)
#define __disable_irq() (spi_test_primask = 1u)
#define __enable_irq() (spi_test_primask = 0u)
#define __set_PRIMASK(value) spi_test_set_primask(value)
#define __DMB() __atomic_thread_fence(__ATOMIC_SEQ_CST)
void Error_Handler(void);
#endif
