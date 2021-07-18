#pragma once

#if !defined(HAL_IMX7)
#error "This header should be included only when building for i.MX7"
#endif

//! @brief Number of available SPI controllers.
//! FIXME: Use all controllers, not only master.
#define HAL_SPI_CHANNEL_COUNT 1
