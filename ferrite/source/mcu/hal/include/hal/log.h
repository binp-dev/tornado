#pragma once

#include "defs.h"
#include "io.h"

#define HAL_LOG_LEVEL_ERROR 1
#define HAL_LOG_LEVEL_WARN 2
#define HAL_LOG_LEVEL_INFO 3
#define HAL_LOG_LEVEL_DEBUG 4

#ifndef HAL_LOG_LEVEL
#error "HAL_LOG_LEVEL must be defined"
#endif

#if (HAL_LOG_LEVEL < HAL_LOG_LEVEL_ERROR) || (HAL_LOG_LEVEL > HAL_LOG_LEVEL_DEBUG)
#error "HAL_LOG_LEVEL has invalid value"
#endif

#define _hal_log(level, format, ...) hal_print("[" level "] " format __HAL_VA_ARGS_WITH_COMMA(__VA_ARGS__))

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_ERROR
#define hal_log_error(...) _hal_log("ERROR", __VA_ARGS__)
#else
#define hal_log_error(...)
#endif

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_WARN
#define hal_log_warn(...) _hal_log("WARN", __VA_ARGS__)
#else
#define hal_log_warn(...)
#endif

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_INFO
#define hal_log_info(...) _hal_log("INFO", __VA_ARGS__)
#else
#define hal_log_info(...)
#endif

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_DEBUG
#define hal_log_debug(...) _hal_log("DEBUG", __VA_ARGS__)
#else
#define hal_log_debug(...)
#endif
