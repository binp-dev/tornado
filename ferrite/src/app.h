#pragma once

#include "app_log.h"


#define APP_FLEXCAN_IRQ_PRIORITY   3
#define APP_GPT_IRQ_PRIORITY       3
#define APP_MU_IRQ_PRIORITY        3


void panic();

#define PANIC() \
    PRINTF("\r\nPANIC %s:%d\r\n", __FILE__, __LINE__); \
    panic()

#define PANIC_(format, ...) \
    PRINTF("\r\nPANIC %s:%d\r\n" format "\r\n", __FILE__, __LINE__ _MAY_EMPTY(__VA_ARGS__)); \
    panic()

#define ASSERT(expr) \
    if (!(expr)) { \
        PANIC_("Assertion failed:\r\n" #expr); \
    } \
    do {} while(0)

#define ASSERT_(expr, format, ...) \
    if (expr) { \
        PANIC_("Assertion failed:\r\n" #expr "\r\n" format, __VA_ARGS__); \
    } \
    do {} while(0)
