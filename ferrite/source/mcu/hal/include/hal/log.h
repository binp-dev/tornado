#pragma once

#include "defs.h"
#include "io.h"

typedef enum {
    HAL_LOG_LEVEL_ERROR = 1,
    HAL_LOG_LEVEL_WARN = 2,
    HAL_LOG_LEVEL_INFO = 3,
    HAL_LOG_LEVEL_DEBUG = 4,
} HalLogLevel;

#ifndef HAL_LOG_LEVEL
#error "HAL_LOG_LEVEL must be defined"
#endif

extern const char *const __hal_log_level_name[6];

#define hal_log(level, format, ...) do { \
    if (HAL_LOG_LEVEL >= level) { \
        hal_debug("[%s] " format, __hal_log_level_name[level] __HAL_VA_ARGS_WITH_COMMA(__VA_ARGS__)); \
    } \
} while(0)

#define hal_log_error(...) hal_log(HAL_LOG_LEVEL_ERROR, __VA_ARGS__) /*! Print error message */
#define hal_log_warn(...)  hal_log(HAL_LOG_LEVEL_WARN,  __VA_ARGS__) /*! Print warning message */
#define hal_log_info(...)  hal_log(HAL_LOG_LEVEL_INFO,  __VA_ARGS__) /*! Print general information */
#define hal_log_debug(...) hal_log(HAL_LOG_LEVEL_DEBUG, __VA_ARGS__) /*! Print debugging information */
