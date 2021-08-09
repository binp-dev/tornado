/*!
 * @file spi.h
 * @brief SPI abstraction layer
 */

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "defs.h"

#if defined(HAL_IMX7)
#include "imx7/spi.h"
#else
#error "Unknown target"
#endif

/*! @brief Initialize SPI subsystem. */
void hal_spi_init();

/*! @brief Denitialize SPI subsystem. */
void hal_spi_deinit();

/*!
 * @brief Enable and configure single SPI controller as master.
 * @param[in] channel SPI controller index.
 * @param[in] baud_rate SPI baudrate.
 * @return Return code.
 */
hal_retcode hal_spi_enable(uint32_t channel, uint32_t baud_rate);

/*!
 * @brief Disable single SPI controller.
 * @param[in] channel SPI controller index.
 * @return Return code.
 */
hal_retcode hal_spi_disable(uint32_t channel);

/*!
 * @brief Transfer data over SPI synchronously.
 * @param[in] channel SPI controller index.
 * @param[in] tx_buf Data to transmit.
 * @param[in] rx_buf Where data to be placed when received.
 * @param[in] len Length of the transfered data.
 * @param[in] timeout Timeout in milliseconds to wait for transfer. 0 - means non-blocking call, HAL_WAIT_FOREVER - wait forever.
 * @return Operation status, zero on success.
 */
hal_retcode hal_spi_xfer(uint32_t channel, uint8_t *tx_buf, uint8_t *rx_buf, size_t len, uint32_t timeout);
