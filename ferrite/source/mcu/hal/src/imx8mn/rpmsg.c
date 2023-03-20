#include <hal/rpmsg.h>
#include <hal/imx8mn/rpmsg.h>

#include "FreeRTOS.h"

#include "rpmsg_lite.h"
#include "rpmsg_queue.h"
#include "rpmsg_ns.h"

#include "board.h"

#define RPMSG_LITE_SHMEM_BASE         ((void *)(VDEV0_VRING_BASE))
#define RPMSG_LITE_LINK_ID            (RL_PLATFORM_IMX8MN_M7_USER_LINK_ID)
#define RPMSG_LITE_NS_ANNOUNCE_STRING "rpmsg-virtual-tty-channel-1"

static struct rpmsg_lite_instance *RPMSG = RL_NULL;

void hal_rpmsg_init() {
    RPMSG = rpmsg_lite_remote_init(RPMSG_LITE_SHMEM_BASE, RPMSG_LITE_LINK_ID, RL_NO_FLAGS);
    while (!rpmsg_lite_is_link_up(RPMSG));
}

void hal_rpmsg_deinit() {
    rpmsg_lite_deinit(RPMSG);
    RPMSG = RL_NULL;
}

hal_retcode hal_rpmsg_create_channel(hal_rpmsg_channel *channel, uint32_t remote_id) {
    channel->remote_addr = 0;
    channel->queue = rpmsg_queue_create(RPMSG);
    if (channel->queue == RL_NULL) {
        return HAL_FAILURE;
    }

    channel->ept = rpmsg_lite_create_ept(RPMSG, remote_id, rpmsg_queue_rx_cb, channel->queue);
    if (channel->ept == RL_NULL) {
        rpmsg_queue_destroy(RPMSG, channel->queue);
        return HAL_FAILURE;
    }

    if (rpmsg_ns_announce(RPMSG, channel->ept, RPMSG_LITE_NS_ANNOUNCE_STRING, RL_NS_CREATE) != RL_SUCCESS) {
        rpmsg_lite_destroy_ept(RPMSG, channel->ept);
        rpmsg_queue_destroy(RPMSG, channel->queue);
        return HAL_FAILURE;
    }

    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_destroy_channel(hal_rpmsg_channel *channel) {
    rpmsg_ns_announce(RPMSG, channel->ept, RPMSG_LITE_NS_ANNOUNCE_STRING, RL_NS_DESTROY);
    rpmsg_lite_destroy_ept(RPMSG, channel->ept);
    rpmsg_queue_destroy(RPMSG, channel->queue);
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_alloc_tx_buffer(hal_rpmsg_channel *channel, uint8_t **tx_buf, size_t *size, uint32_t timeout) {
    uint32_t size_uint = 0;
    char *buffer = rpmsg_lite_alloc_tx_buffer(RPMSG, &size_uint, timeout);
    if (buffer == RL_NULL) {
        return HAL_BAD_ALLOC;
    }
    *size = (size_t)size_uint;
    *tx_buf = (uint8_t*)buffer;
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_free_rx_buffer(hal_rpmsg_channel *channel, uint8_t *rx_buf) {
    int ret = rpmsg_queue_nocopy_free(RPMSG, (char*)rx_buf);
    if (ret != RL_SUCCESS) {
        /// TODO: Handle different error types.
        return HAL_FAILURE;
    }
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_send_nocopy(hal_rpmsg_channel *channel, uint8_t *tx_buf, size_t len) {
    /// TODO: Check that we have set channel->remote_address
    int ret = rpmsg_lite_send_nocopy(RPMSG, channel->ept, channel->remote_addr, (char*)tx_buf, (uint32_t)len);
    if (ret != RL_SUCCESS) {
        /// TODO: Handle different error types.
        return HAL_FAILURE;
    }
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_recv_nocopy(hal_rpmsg_channel *channel, uint8_t **rx_buf, size_t *len, uint32_t timeout) {
    uint32_t len_uint = 0;
    int ret = rpmsg_queue_recv_nocopy(RPMSG, channel->queue, &channel->remote_addr, (char**)rx_buf, &len_uint, timeout);
    if (ret != RL_SUCCESS) {
        /// TODO: Handle different error types.
        return HAL_FAILURE;
    }
    *len = (size_t)len_uint;
    return HAL_SUCCESS;
}
