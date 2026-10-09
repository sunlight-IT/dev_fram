#ifndef __PORT_H__
#define __PORT_H__

#include "usr_common.h"

typedef struct
{
  void *port;
  uint16_t pin;
  bool has_irq;
} gpio_pin_t;

#endif
