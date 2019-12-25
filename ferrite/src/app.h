#pragma once

#include "app_log.h"


#define APP_FLEXCAN_IRQ_PRIORITY   3
#define APP_GPT_IRQ_PRIORITY       3
#define APP_MU_IRQ_PRIORITY        3


void panic();

#define PANIC(format, ...) \
    PRINTF("\r\nPANIC: \r\n" format "\r\n" _VA_ARGS_MAY_EMPTY(__VA_ARGS__)); \
    panic()
