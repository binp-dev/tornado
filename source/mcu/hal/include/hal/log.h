#pragma once

#include "defs.h"
#include "io.h"

#define HAL_LOG_LEVEL_FATAL 0
#define HAL_LOG_LEVEL_ERROR 1
#define HAL_LOG_LEVEL_WARN 2
#define HAL_LOG_LEVEL_INFO 3
#define HAL_LOG_LEVEL_DEBUG 4
#define HAL_LOG_LEVEL_TRACE 5

#ifndef HAL_LOG_LEVEL
#error "HAL_LOG_LEVEL must be defined"
#endif

#if (HAL_LOG_LEVEL < HAL_LOG_LEVEL_FATAL) || (HAL_LOG_LEVEL > HAL_LOG_LEVEL_TRACE)
#error "HAL_LOG_LEVEL has invalid value"
#endif

#define _hal_log(level, format, ...) hal_print("[mcu:" level "] " format __HAL_VA_ARGS_WITH_COMMA(__VA_ARGS__))

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_ERROR
#define hal_log_fatal(...) _hal_log("fatal", __VA_ARGS__)
#else
#define hal_log_fatal(...)
#endif

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_ERROR
#define hal_log_error(...) _hal_log("error", __VA_ARGS__)
#else
#define hal_log_error(...)
#endif

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_WARN
#define hal_log_warn(...) _hal_log("warning", __VA_ARGS__)
#else
#define hal_log_warn(...)
#endif

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_INFO
#define hal_log_info(...) _hal_log("info", __VA_ARGS__)
#else
#define hal_log_info(...)
#endif

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_DEBUG
#define hal_log_debug(...) _hal_log("debug", __VA_ARGS__)
#else
#define hal_log_debug(...)
#endif

#if HAL_LOG_LEVEL >= HAL_LOG_LEVEL_TRACE
#define hal_log_trace(...) _hal_log("trace", __VA_ARGS__)
#else
#define hal_log_trace(...)
#endif
