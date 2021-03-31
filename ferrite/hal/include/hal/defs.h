#pragma once

#include <stdint.h>

// Timeout special values

#define HAL_NON_BLOCK ((uint32_t)0)
#define HAL_WAIT_FOREVER ((uint32_t)0xFFFFFFFF)

// Return codes

typedef uint8_t hal_retcode;

#define HAL_SUCCESS ((hal_retcode)0)
#define HAL_TIMED_OUT ((hal_retcode)1)
// ...
