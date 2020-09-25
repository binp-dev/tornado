/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "board.h"
#include "debug_console_imx.h"
#include "mu_imx.h"

#include "FreeRTOS.h"
#include "rpmsg/rpmsg_rtos.h"

#include "app.h"
#include "app_rpmsg.h"
#include "app_log.h"


static struct remote_device *rdev = NULL;
static struct rpmsg_channel *app_chnl = NULL;


/*! MU Interrrupt ISR */
void BOARD_MU_HANDLER(void) {
    /* calls into rpmsg_handler provided by middleware */
    rpmsg_handler();
}

void APP_RPMSG_HardwareInit() {
    /* RDC MU*/
    RDC_SetPdapAccess(RDC, BOARD_MU_RDC_PDAP, 3 << (BOARD_DOMAIN_ID * 2), false, false);
    /* Enable clock gate for MU*/
    CCM_ControlGate(CCM, BOARD_MU_CCM_CCGR, ccmClockNeededRun);
}

uint8_t APP_RPMSG_Init() {
    /*
     * Prepare for the MU Interrupt
     *  MU must be initialized before rpmsg init is called
     */
    MU_Init(BOARD_MU_BASE_ADDR);
    NVIC_SetPriority(BOARD_MU_IRQ_NUM, APP_MU_IRQ_PRIORITY);
    NVIC_EnableIRQ(BOARD_MU_IRQ_NUM);


    if (rpmsg_rtos_init(0 /*REMOTE_CPU_ID*/, &rdev, RPMSG_MASTER, &app_chnl) != 0) {
        APP_ERROR("Cannot initialize RPMSG");
        goto rpmsg_deinit;
    }

    APP_INFO(
        "Name service handshake is done, M4 has setup a rpmsg channel [%d ---> %d]",
        app_chnl->src, app_chnl->dst
    );

    return 0;

rpmsg_deinit:
    NVIC_DisableIRQ(BOARD_MU_IRQ_NUM);

    return 1;
}

uint8_t APP_RPMSG_Deinit() {
    rpmsg_rtos_deinit(rdev);
    NVIC_DisableIRQ(BOARD_MU_IRQ_NUM);
    return 0;
}

int32_t APP_RPMSG_Send(const uint8_t *data, uint32_t len) {
    return rpmsg_rtos_send(app_chnl->rp_ept, (void *)data, len, app_chnl->dst);
}

int32_t APP_RPMSG_Receive(uint8_t *data, uint32_t *len, uint32_t maxlen, uint32_t timeout) {
    return rpmsg_rtos_recv(app_chnl->rp_ept, (void *)data, (int*)len, maxlen, NULL, timeout);
}
