#pragma once

#define APP_LOG_LEVEL_ERROR 1
#define APP_LOG_LEVEL_WARN  2
#define APP_LOG_LEVEL_INFO  3
#define APP_LOG_LEVEL_DEBUG 4
#define APP_LOG_LEVEL_TRACE 5

extern const char *const APP_LOG_LEVEL_NAME[6];

#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL APP_LOG_LEVEL_INFO
#endif // APP_LOG_LEVEL

#define _APP_VA_ARGS_MAY_EMPTY(...) , ##__VA_ARGS__

/*! Print log message with specified priority */
#define APP_LOG(level, format, ...) \
    if (APP_LOG_LEVEL >= level) { \
        PRINTF("[%s]" format "\r\n", APP_LOG_LEVEL_NAME[level] _APP_VA_ARGS_MAY_EMPTY(__VA_ARGS__)); \
    } \
    do {} while(0)

#define APP_ERROR(...) APP_LOG(APP_LOG_LEVEL_ERROR, __VA_ARGS__) /*! Print error message */
#define APP_WARN(...)  APP_LOG(APP_LOG_LEVEL_WARN,  __VA_ARGS__) /*! Print warning message */
#define APP_INFO(...)  APP_LOG(APP_LOG_LEVEL_INFO,  __VA_ARGS__) /*! Print general information */
#define APP_DEBUG(...) APP_LOG(APP_LOG_LEVEL_DEBUG, __VA_ARGS__) /*! Print debugging information */
#define APP_TRACE(...) APP_LOG(APP_LOG_LEVEL_TRACE, __VA_ARGS__) /*! Print trace */
