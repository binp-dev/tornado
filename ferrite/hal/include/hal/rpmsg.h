#pragma once

#include <stdlib.h>
#include <stdint.h>

/*! @brief RPMSG endpoint opaque handle. */
typedef struct hal_rpmsg_ept hal_rpmsg_ept;

/*! @brief Initialize RPMSG subsystem. */
void hal_rpmsg_init();

/*! @brief Deinitialize RPMSG subsystem. */
void hal_rpmsg_deinit();

/*!
 * @brief Create RPMSG endpoint.
 * @param[out] ept Pointer to store endpoint handle.
 * @param[in] addr Desired address of endpoint. 
 * @return Status, 0 on success.
 */
int32_t hal_rpmsg_create_ept(hal_rpmsg_ept **ept, uint32_t addr);

/*!
 * @brief Destroy RPMSG endpoint.
 * @param[in] ept Endpoint handle to destroy.
 * @return Status, 0 on success.
 */
int32_t hal_rpmsg_destroy_ept(hal_rpmsg_ept *ept);

/*!
 * @brief Receive RPMSG message without data copying.
 * @param[in] ept Endpoint handle.
 * @param[out] src Address of remote endpoint.
 * @param[out] rx_buf Pointer to shared memory where received data is stored.
 * @param[out] len Length of received data.
 * @param[in] timeout Timeout in milliseconds to wait for message. 0 - means non-blocking call, 0xFFFFFFFF - wait forever.
 * @return Status, 0 on success.
 */
int32_t hal_rpmsg_recv_nocopy(hal_rpmsg_ept *ept, uint32_t *src, uint8_t **rx_buf, size_t *len, uint32_t timeout);

/*!
 * @brief Free shared memory of received message.
 * @param[in] rx_buf Pointer to shared memory where received data was stored.
 * @return Status, 0 on success.
 */
int32_t hal_rpmsg_free_rx_buffer(uint8_t *rx_buf);

/*!
 * @brief Allocate shared memory for message sending.
 * @param[out] tx_buf Pointer to allocated shared memory.
 * @param[out] size Size of allocated buffer.
 * @param[in] timeout Timeout in milliseconds to wait for allocation. 0 - means non-blocking call, 0xFFFFFFFF - wait forever.
 * @return Status, 0 on success.
 */
int32_t hal_rpmsg_alloc_tx_buffer(uint8_t **tx_buf, size_t *size, uint32_t timeout);

/*!
 * @brief Receive RPMSG message without data copying.
 * @param[in] ept Endpoint handle.
 * @param[in] dst Address of remote endpoint.
 * @param[in] tx_buf Pointer to shared memory with data to send.
 * @param[in] len Length of sending data.
 * @return Status, 0 on success.
 */
int32_t hal_rpmsg_send_nocopy(hal_rpmsg_ept *ept, uint32_t dst, uint8_t *tx_buf, uint32_t len);
