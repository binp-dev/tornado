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


#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "flexcan.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app.h"
#include "app_debug.h"
#include "app_flexcan.h"
#include "app_log.h"
#include "app_time.h"


#define ms_to_ticks APP_Ms2Ticks

#define TX_MSG_BUF_NUM    8
#define RX_MSG_BUF_NUM    9


static const flexcan_timing_t timing_table[] = {
    {7, 3, 7, 7, 6},  /* 125 kHz from 24 MHz OSC */
    {3, 3, 7, 7, 6},  /* 250 kHz from 24 MHz OSC */
    {1, 3, 7, 7, 6},  /* 500 kHz from 24 MHz OSC */
    {0, 3, 7, 7, 6},  /* 1   MHz from 24 MHz OSC */
};

static const char *const flexcan_rate_text[] = {
    "125 kHz",
    "250 kHz",
    "500 kHz",
    "1 MHz"
};


static volatile flexcan_msgbuf_t *txMsgBufPtr;
static volatile flexcan_msgbuf_t *rxMsgBufPtr;

static volatile flexcan_msgbuf_t rxBuffer;
static volatile bool rxBufferLocked;
static volatile bool rxWasMissed;

static SemaphoreHandle_t txSemaphore = NULL;
static SemaphoreHandle_t rxSemaphore = NULL;


/*! @brief FLEXCAN interrupt handler. */
void BOARD_FLEXCAN_HANDLER();


void APP_FLEXCAN_HardwareInit() {
    /* In this example, we need to grasp board flexcan exclusively */
    RDC_SetPdapAccess(RDC, BOARD_FLEXCAN_RDC_PDAP, 3 << (BOARD_DOMAIN_ID * 2), false, false);

    /* Select board flexcan derived from OSC clock(24M) */
    CCM_UpdateRoot(CCM, BOARD_FLEXCAN_CCM_ROOT, ccmRootmuxUartOsc24m, 0, 0);
    /* Enable flexcan clock */
    CCM_EnableRoot(CCM, BOARD_FLEXCAN_CCM_ROOT);
    CCM_ControlGate(CCM, BOARD_FLEXCAN_CCM_CCGR, ccmClockNeededRunWait);

    /* FLEXCAN Pin setting */
    configure_flexcan_pins(BOARD_FLEXCAN_BASEADDR);
}


uint8_t APP_FLEXCAN_Init(APP_FLEXCAN_Baudrate rate_id, uint16_t rx_mask) {
    flexcan_init_config_t initConfig = {
        .timing = timing_table[rate_id],
        .operatingMode = flexcanNormalMode,
        .maxMsgBufNum  = 16
    };

    /* Initialize FlexCAN module. */
    FLEXCAN_Init(BOARD_FLEXCAN_BASEADDR, &initConfig);
    /* Enable FlexCAN Clock. */
    FLEXCAN_Enable(BOARD_FLEXCAN_BASEADDR);
    /* Set FlexCAN to use Global mask mode. */
    FLEXCAN_SetRxMaskMode(BOARD_FLEXCAN_BASEADDR, flexcanRxMaskGlobal);
    /* Set FlexCAN global mask. */
    FLEXCAN_SetRxGlobalMask(BOARD_FLEXCAN_BASEADDR, ~CAN_ID_STD(rx_mask));

    /* Clear Tx and Rx message buffer interrupt pending bit. */
    FLEXCAN_ClearMsgBufStatusFlag(BOARD_FLEXCAN_BASEADDR, TX_MSG_BUF_NUM);
    FLEXCAN_ClearMsgBufStatusFlag(BOARD_FLEXCAN_BASEADDR, RX_MSG_BUF_NUM);
    /* Enable Tx and Rx message buffer interrupt. */
    FLEXCAN_SetMsgBufIntCmd(BOARD_FLEXCAN_BASEADDR, TX_MSG_BUF_NUM, true);
    FLEXCAN_SetMsgBufIntCmd(BOARD_FLEXCAN_BASEADDR, RX_MSG_BUF_NUM, true);

    /* Initialize Global variable. */
    txSemaphore = xSemaphoreCreateBinary();
    rxSemaphore = xSemaphoreCreateBinary();
    if (!txSemaphore || !rxSemaphore) {
        return 1;
    }

    rxBufferLocked = false;
    rxWasMissed = false;

    txMsgBufPtr = FLEXCAN_GetMsgBufPtr(BOARD_FLEXCAN_BASEADDR, TX_MSG_BUF_NUM);
    rxMsgBufPtr = FLEXCAN_GetMsgBufPtr(BOARD_FLEXCAN_BASEADDR, RX_MSG_BUF_NUM);

    /* Setup Rx MsgBuf to receive Frame. */
    rxMsgBufPtr->idStd = 0x0;
    rxMsgBufPtr->code  = flexcanRxEmpty;

    txMsgBufPtr->prio  = 0x0; /* We don't use local priority */
    txMsgBufPtr->idStd = 0x0; /* Set Tx Identifier */
    txMsgBufPtr->idExt = 0x0; /* We don't use Extend Id. */
    txMsgBufPtr->dlc   = 0x8; /* Send 8 bytes of data. */
    txMsgBufPtr->rtr   = 0x0; /* Send data frame. */
    txMsgBufPtr->ide   = 0x0; /* Frame format is standard. */
    txMsgBufPtr->srr   = 0x1; /* Don't care in standard id mode. */

    /* Set FlexCAN interrupt priority. */
    NVIC_SetPriority(BOARD_FLEXCAN_IRQ_NUM, APP_FLEXCAN_IRQ_PRIORITY);
    /* Enable FlexCAN interrupt. */
    NVIC_EnableIRQ(BOARD_FLEXCAN_IRQ_NUM);

    APP_INFO("FLEXCAN Initialized");
    APP_INFO("  Message format: Standard (11 bit id)");
    APP_INFO("  Message buffer %d used for Rx.", RX_MSG_BUF_NUM);
    APP_INFO("  Message buffer %d used for Tx.", TX_MSG_BUF_NUM);
    APP_INFO("  Interrupt mode: Enabled");
    APP_INFO("  Operating mode: TX and RX --> Normal");
    APP_INFO("  Baud rate: %s", flexcan_rate_text[rate_id]);

    return 0;
}

void BOARD_FLEXCAN_HANDLER() {
    BaseType_t txHptw = pdFALSE, rxHptw = pdFALSE;
    
    /* Solve Tx interrupt */
    if (FLEXCAN_GetMsgBufStatusFlag(BOARD_FLEXCAN_BASEADDR, TX_MSG_BUF_NUM))
    {
        /* Notify sender */
        xSemaphoreGiveFromISR(txSemaphore, &txHptw);

        FLEXCAN_ClearMsgBufStatusFlag(BOARD_FLEXCAN_BASEADDR, TX_MSG_BUF_NUM);
    }

    /* Solve Rx interrupt */
    if (FLEXCAN_GetMsgBufStatusFlag(BOARD_FLEXCAN_BASEADDR, RX_MSG_BUF_NUM))
    {
        /* Lock message buffer for receive data. */
        FLEXCAN_LockRxMsgBuf(BOARD_FLEXCAN_BASEADDR, RX_MSG_BUF_NUM);
        
        /* Check if ready to receive a new frame */
        if (!rxBufferLocked) {
            /* Copy data */
            rxBuffer = *rxMsgBufPtr;
            /* Notify receiver */
            if (xSemaphoreGiveFromISR(rxSemaphore, &rxHptw) != pdTRUE) {
                rxWasMissed = true;
            }
        } else {
            rxWasMissed = true;
        }

        FLEXCAN_UnlockAllRxMsgBuf(BOARD_FLEXCAN_BASEADDR);

        FLEXCAN_ClearMsgBufStatusFlag(BOARD_FLEXCAN_BASEADDR, RX_MSG_BUF_NUM);
    }

    /* Yield to higher priority task */
    portYIELD_FROM_ISR(txHptw || rxHptw);
}

uint8_t APP_FLEXCAN_Send(const APP_FLEXCAN_Frame *frame, uint32_t timeout) {
    uint8_t status = 0;

    /* Set ID and length. */
    txMsgBufPtr->idStd = frame->id;
    txMsgBufPtr->dlc = frame->len;

    /* Load data to message buf. */
    if (frame->len <= 4) {
        memcpy(
            (uint8_t*)&txMsgBufPtr->word0 - frame->len + 4,
            frame->data,
            frame->len
        );
    } else {
        memcpy(
            (uint8_t*)&txMsgBufPtr->word0,
            frame->data + frame->len - 4,
            4
        );
        memcpy(
            (uint8_t*)&txMsgBufPtr->word1 - frame->len + 8,
            frame->data,
            frame->len - 4
        );
    }

    /* Start transmit. */
    txMsgBufPtr->code  = flexcanTxDataOrRemte;

    /* Wait for send to complete */
    if (xSemaphoreTake(txSemaphore, ms_to_ticks(timeout)) != pdTRUE) {
        /* FIXME: Recover from timeout. */
        APP_ERROR("FLEXCAN Tx timed out");
        status = 1;
    }

    return status;
}

uint8_t APP_FLEXCAN_Receive(APP_FLEXCAN_Frame *frame, uint32_t timeout) {
    uint8_t status = 0;

    /* Wait for response */
    if (xSemaphoreTake(rxSemaphore, ms_to_ticks(timeout)) != pdTRUE) {
        status = 1;
        goto timed_out;
    }

    rxBufferLocked = true;

    /* Set ID and length. */
    frame->id = rxBuffer.idStd;
    frame->len = rxBuffer.dlc;

    /* Load data from message buf. */
    if (frame->len <= 4) {
        memcpy(
            frame->data,
            (uint8_t*)&rxBuffer.word0 - frame->len + 4,
            frame->len
        );
    } else {
        memcpy(
            frame->data + frame->len - 4,
            (uint8_t*)&rxBuffer.word0,
            4
        );
        memcpy(
            frame->data,
            (uint8_t*)&rxBuffer.word1 - frame->len + 8,
            frame->len - 4
        );
    }

    rxBufferLocked = false;

    if (rxWasMissed) {
        APP_WARN("Some CAN frames was missed");
        rxWasMissed = false;
    }

timed_out:

    return status;
}
