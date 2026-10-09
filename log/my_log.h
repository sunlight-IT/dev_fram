#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
// #include "dev_uart.h"

// #include "stm32f1xx.h"
typedef enum  {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
}log_level_t;

void zlog_set_log_level(log_level_t level);


int __io_putchar(int ch);
__attribute__((used)) int _write(int fd, char* buf, int size);

void zlog(const char* fmt, ...);

uint32_t zlog_get_tick(void);

extern log_level_t s_log_level;

static inline bool zlog_log_level_is_valid(log_level_t level)
{
    return (level >= LOG_DEBUG) && (level <= LOG_ERROR);
}

static inline bool zlog_apply_log_level(log_level_t level)
{
    if (!zlog_log_level_is_valid(level))
    {
        return false;
    }
    zlog_set_log_level(level);
    return true;
}

static inline log_level_t zlog_get_log_level(void)
{
    return s_log_level;
}
#define LOGI(fmt, ...) do { \
    if (s_log_level <= LOG_INFO) { \
        zlog("[%09lu] [INFO] <%-20s> : " fmt " \r\n", zlog_get_tick(), __func__, ##__VA_ARGS__); \
    } \
} while (0)
#define LOGW(fmt, ...) do { \
    if (s_log_level <= LOG_WARN) { \
        zlog("[%09lu] [WARN] <%-20s> : " fmt " \r\n", zlog_get_tick(), __func__, ##__VA_ARGS__); \
    } \
} while (0)
#define LOGD(fmt, ...) do { \
    if (s_log_level <= LOG_DEBUG) { \
        zlog("[%09lu] [DEBUG] <%-20s> : " fmt " \r\n", zlog_get_tick(), __func__, ##__VA_ARGS__); \
    } \
} while (0)
#define LOGE(fmt, ...) do { \
    if (s_log_level <= LOG_ERROR) { \
        zlog("[%09lu] [EROR] <%-20s> : " fmt " \r\n", zlog_get_tick(), __func__, ##__VA_ARGS__); \
    } \
} while (0)

#define vofa_LOGI(TYPE, fmt, ...) printf(" %s:" fmt "\n", TYPE, ##__VA_ARGS__)

void zlog_init(void* uart);
