/**
 * @file    usr_check.h
 * @brief   Statement-safe guard macros for the Usr driver framework.
 */
#ifndef __USR_CHECK_H__
#define __USR_CHECK_H__

#include "usr_common.h"

/* A true condition means that the guarded operation has failed. */
#define USR_RETURN_IF(condition, return_value) \
  do \
  { \
    if (condition) \
    { \
      return (return_value); \
    } \
  } while (0)

#define USR_RETURN_VOID_IF(condition) \
  do \
  { \
    if (condition) \
    { \
      return; \
    } \
  } while (0)

#define USR_RETURN_STATUS_IF_NOT_OK(status_variable) \
  do \
  { \
    if ((status_variable) != USR_OK) \
    { \
      return (status_variable); \
    } \
  } while (0)

#define USR_RETURN_IF_LOG(condition, return_value) \
  do \
  { \
    if (condition) \
    { \
      LOGE("guard failed: %s", #condition); \
      return (return_value); \
    } \
  } while (0)

#define USR_RETURN_VOID_IF_LOG(condition) \
  do \
  { \
    if (condition) \
    { \
      LOGE("guard failed: %s", #condition); \
      return; \
    } \
  } while (0)

#endif /* __USR_CHECK_H__ */
