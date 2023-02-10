#pragma once

#include <stdint.h>

// Timeout special values

#define HAL_NON_BLOCK ((uint32_t)0)
#define HAL_WAIT_FOREVER ((uint32_t)0xFFFFFFFF)

// Return codes

typedef uint8_t hal_retcode;

// clang-format off
#define HAL_SUCCESS            ((hal_retcode)0x00) // Success
#define HAL_FAILURE            ((hal_retcode)0x01) // Generic failure
#define HAL_BAD_ALLOC          ((hal_retcode)0x02) // Memory allocation failure
#define HAL_OUT_OF_BOUNDS      ((hal_retcode)0x03) // Try to access element out of container bounds.
#define HAL_INVALID_INPUT      ((hal_retcode)0x04) // User provided invalid input.
#define HAL_INVALID_DATA       ((hal_retcode)0x05) // Invalid data generated during process.
// ...
#define HAL_UNIMPLEMENTED      ((hal_retcode)0xFE) // Functionality isn't implemented yet
#define HAL_TIMED_OUT          ((hal_retcode)0xFF) // Timeout exceeded
// clang-format on

const char *hal_retcode_str(hal_retcode code);

// Helper macros

#ifdef __GNUC__
#define __HAL_VA_ARGS_WITH_COMMA(...) , ##__VA_ARGS__
#else
#error "Compilers other than GCC are not supported yet"
#endif
