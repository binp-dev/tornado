/*!
 * @file rpmsg.h
 * @brief RPMSG abstraction layer
 */
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "defs.h"

#if defined(HAL_IMX7)
#include "imx7/rpmsg.h"
#elif defined(HAL_IMX8MN)
#include "imx8mn/rpmsg.h"
#else
#error "Unknown target"
#endif

/*! @brief RPMSG channel handle. */
typedef struct hal_rpmsg_channel hal_rpmsg_channel;

/*! @brief Initialize RPMSG subsystem. */
void hal_rpmsg_init();

/*! @brief Deinitialize RPMSG subsystem. */
void hal_rpmsg_deinit();

/*!
 * @brief Create RPMSG channel.
 * @param[out] channel Pointer to store channel handle.
 * @param[in] remote_id Remote device ID.
 * @return Return code (see `defs.h`).
 */
hal_retcode hal_rpmsg_create_channel(hal_rpmsg_channel *channel, uint32_t remote_id);

/*!
 * @brief Destroy RPMSG channel.
 * @param[in] channel Channel handle to destroy.
 * @return Return code (see `defs.h`).
 */
hal_retcode hal_rpmsg_destroy_channel(hal_rpmsg_channel *channel);

/*!
 * @brief Allocate shared memory for message sending.
 * @param[in] channel Channel handle.
 * @param[out] tx_buf Pointer to allocated shared memory.
 * @param[out] size Size of allocated buffer.
 * @param[in] timeout Timeout in milliseconds to wait for allocation. 0 - means non-blocking call, HAL_WAIT_FOREVER - wait forever.
 * @return Return code (see `defs.h`).
 */
hal_retcode hal_rpmsg_alloc_tx_buffer(hal_rpmsg_channel *channel, uint8_t **tx_buf, size_t *size, uint32_t timeout);

/*!
 * @brief Free received message buffer.
 * @param[in] channel Channel handle.
 * @param[in] rx_buf Pointer to shared memory of received buffer.
 * @return Return code (see `defs.h`).
 */
hal_retcode hal_rpmsg_free_rx_buffer(hal_rpmsg_channel *channel, uint8_t *rx_buf);

/*!
 * @brief Send RPMSG message without data copying.
 * @param[in] channel Channel handle.
 * @param[in] tx_buf Pointer to shared memory with data to send.
 * @param[in] len Length of sending data.
 * @return Return code (see `defs.h`).
 */
hal_retcode hal_rpmsg_send_nocopy(hal_rpmsg_channel *channel, uint8_t *tx_buf, size_t len);

/*!
 * @brief Receive RPMSG message without data copying.
 * @param[in] channel Channel handle.
 * @param[out] rx_buf Pointer to shared memory where received data is stored.
 * @param[out] len Length of received data.
 * @param[in] timeout Timeout in milliseconds to wait for message. 0 - means non-blocking call, HAL_WAIT_FOREVER - wait forever.
 * @return Return code (see `defs.h`).
 */
hal_retcode hal_rpmsg_recv_nocopy(hal_rpmsg_channel *channel, uint8_t **rx_buf, size_t *len, uint32_t timeout);
