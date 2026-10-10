/**
  ******************************************************************************
  * @file    usr_port_platform.c
  * @brief   Demo platform init — no-op stubs.
  *          Actual projects initialise CubeMX peripherals here.
  ******************************************************************************
  */
#include "usr_port.h"

usr_status_t usr_port_platform_init(void)
{
  return USR_OK;
}

usr_status_t usr_port_get_system_info(usr_port_system_info_t *info)
{
  if (info == NULL)
  {
    return USR_ERR_PARAM;
  }

  info->hal_version   = 0u;
  info->sys_clock_hz  = 0u;
  return USR_OK;
}

void usr_port_system_reset(uint32_t delay_ms)
{
  (void)delay_ms;
}