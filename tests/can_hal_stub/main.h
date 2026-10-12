#ifndef CAN_TEST_MAIN_H
#define CAN_TEST_MAIN_H

#include "board_support.h"

extern uint32_t can_test_primask;
#define __get_PRIMASK() (can_test_primask)
#define __disable_irq() (can_test_primask = 1u)
#define __enable_irq() (can_test_primask = 0u)
#define __DMB() __atomic_thread_fence(__ATOMIC_SEQ_CST)

void Error_Handler(void);

#endif
