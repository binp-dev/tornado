#pragma once

#include <stdint.h>

#include "FreeRTOS.h"


#define APP_FOREVER_MS         ((uint32_t)(0xFFFFFFFFul))

TickType_t APP_Ms2Ticks(uint32_t ms);
