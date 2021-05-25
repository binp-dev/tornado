#include "config.h"

#include <hal/rpmsg.h>
#include "rpmsg.h"

#include <board.h>
#include <mu_imx.h>

#include <FreeRTOS.h>
#include <rpmsg/rpmsg_rtos.h>

/*! MU Interrrupt ISR */
void BOARD_MU_HANDLER(void) {
    /* calls into rpmsg_handler provided by middleware */
    rpmsg_handler();
}

void hal_rpmsg_init() {
    /* RDC MU*/
    RDC_SetPdapAccess(RDC, BOARD_MU_RDC_PDAP, 3 << (BOARD_DOMAIN_ID * 2), false, false);
    /* Enable clock gate for MU*/
    CCM_ControlGate(CCM, BOARD_MU_CCM_CCGR, ccmClockNeededRun);

    /*
     * Prepare for the MU Interrupt
     * MU must be initialized before rpmsg init is called
     */
    MU_Init(BOARD_MU_BASE_ADDR);
    NVIC_SetPriority(BOARD_MU_IRQ_NUM, HAL_MU_IRQ_PRIORITY);
    NVIC_EnableIRQ(BOARD_MU_IRQ_NUM);
}

void hal_rpmsg_deinit() {
    NVIC_DisableIRQ(BOARD_MU_IRQ_NUM);
}

hal_retcode hal_rpmsg_create_channel(hal_rpmsg_channel *channel, uint32_t remote_id) {
    int ret = rpmsg_rtos_init(remote_id /*REMOTE_CPU_ID*/, &channel->rdev, RPMSG_MASTER, &channel->app_chnl);
    if (ret != RPMSG_SUCCESS) {
        //log_error("Cannot initialize RPMSG");
        return HAL_FAILURE;
    }
    /*
    log_info(
        "Name service handshake is done, M4 has setup a rpmsg channel [%d ---> %d]",
        channel->app_chnl->src, channel->app_chnl->dst
    );
    */
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_destroy_channel(hal_rpmsg_channel *channel) {
    rpmsg_rtos_deinit(channel->rdev);
    /// FIXME: should we deinit `channel->app_chnl`?
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_alloc_tx_buffer(hal_rpmsg_channel *channel, uint8_t **tx_buf, size_t *size, uint32_t timeout) {
    /// TODO: Handle timeout.
    unsigned long size_ulong = 0;
    void *buffer = rpmsg_rtos_alloc_tx_buffer(channel->app_chnl->rp_ept, &size_ulong);
    if (buffer == NULL) {
        return HAL_BAD_ALLOC;
    }
    *size = (size_t)size_ulong;
    *tx_buf = (uint8_t*)buffer;
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_free_rx_buffer(hal_rpmsg_channel *channel, uint8_t *rx_buf) {
    int ret = rpmsg_rtos_recv_nocopy_free(channel->app_chnl->rp_ept, (void*)rx_buf);
    if (ret != RPMSG_SUCCESS) {
        /// TODO: Handle different error types.
        return HAL_FAILURE;
    }
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_send_nocopy(hal_rpmsg_channel *channel, uint8_t *tx_buf, size_t len) {
    int ret = rpmsg_rtos_send_nocopy(channel->app_chnl->rp_ept, (void*)tx_buf, (int)len, channel->app_chnl->dst);
    if (ret != RPMSG_SUCCESS) {
        /// TODO: Handle different error types.
        return HAL_FAILURE;
    }
    return HAL_SUCCESS;
}

hal_retcode hal_rpmsg_recv_nocopy(hal_rpmsg_channel *channel, uint8_t **rx_buf, size_t *len, uint32_t timeout) {
    int len_int = 0;
    unsigned long src = 0;
    int ret = rpmsg_rtos_recv_nocopy(channel->app_chnl->rp_ept, (void **)rx_buf, &len_int, &src, timeout);
    if (ret != RPMSG_SUCCESS) {
        /// TODO: Handle different error types.
        return HAL_FAILURE;
    }
    /// TODO: Check message source or return it to user.
    /*
    if (src != channel->app_chnl->src) {
        return HAL_FAILURE;
    }
    */
    *len = (size_t)len_int;
    return HAL_SUCCESS;
}
