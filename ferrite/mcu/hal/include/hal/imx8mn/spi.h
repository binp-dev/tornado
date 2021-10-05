#pragma once

#if !defined(HAL_IMX8MN)
#error "This header should be included only when building for i.MX8M Nano"
#endif

#include <stdint.h>

//! @brief Number of available SPI controllers.
//! FIXME: Use all controllers, not only master.
#define HAL_SPI_CHANNEL_COUNT 1

typedef uint32_t hal_spi_buffer;
