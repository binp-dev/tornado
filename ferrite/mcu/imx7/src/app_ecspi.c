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

#include "board.h"
#include "ecspi.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app.h"
#include "app_debug.h"
#include "app_log.h"
#include "app_time.h"
#include "app_ecspi.h"


#define ms_to_ticks APP_Ms2Ticks


/* define ECSPI master mode parameters configuration. */
#define ECSPI_MASTER_BURSTLENGTH        (7)
#define ECSPI_MASTER_STARTMODE          (0)


typedef struct EcspiState {
    uint8_t*                  txBuffPtr;    /* Pointer to ECSPI Transmit Buffer */
    uint8_t                   txSize;       /* The remaining number of bytes to be transmitted */
    uint8_t*                  rxBuffPtr;    /* Pointer to ECSPI Receive Buffer */
    uint8_t                   rxSize;       /* The remaining number of bytes to be received */
    volatile bool             isBusy;       /* True if there is a active transfer */
} ecspi_state_t;

/* ECSPI runtime state structure */
static volatile ecspi_state_t ecspiState;

static SemaphoreHandle_t xferSemaphore = NULL;


static bool _ECSPI_TransmitBurst();
static bool _ECSPI_ReceiveBurst();
//static bool _ECSPI_GetTransferStatus();
static void _ECSPI_Config(ecspi_init_config_t* initConfig);

/*! @brief ECSPI interrupt handler. */
void BOARD_ECSPI_MASTER_HANDLER();


void APP_ECSPI_HardwareInit() {
    /* RDC ECSPI */
    RDC_SetPdapAccess(RDC, BOARD_ECSPI_MASTER_RDC_PDAP, 3 << (BOARD_DOMAIN_ID * 2), false, false);
    /* Select board ecspi clock derived from OSC clock(24M) */
    CCM_UpdateRoot(CCM, BOARD_ECSPI_MASTER_CCM_ROOT, ccmRootmuxEcspiOsc24m, 0, 0);
    /* Enable ecspi clock gate */
    CCM_EnableRoot(CCM, BOARD_ECSPI_MASTER_CCM_ROOT);
    CCM_ControlGate(CCM, BOARD_ECSPI_MASTER_CCM_CCGR, ccmClockNeededRunWait);
    /* Configure ecspi pin IOMUX */
    configure_ecspi_pins(BOARD_ECSPI_MASTER_BASEADDR);
}


uint8_t APP_ECSPI_Init(uint32_t baud_rate) {
    ecspi_init_config_t ecspiMasterInitConfig = {
        .baudRate = baud_rate,
        .mode = ecspiMasterMode,
        .burstLength = ECSPI_MASTER_BURSTLENGTH,
        .channelSelect = BOARD_ECSPI_MASTER_CHANNEL,
        .clockPhase = ecspiClockPhaseSecondEdge,
        .clockPolarity = ecspiClockPolarityActiveHigh,
        .ecspiAutoStart = ECSPI_MASTER_STARTMODE
    };

    /* Update clock frequency of this module */
    ecspiMasterInitConfig.clockRate = get_ecspi_clock_freq(BOARD_ECSPI_MASTER_BASEADDR);

    /* Ecspi module initialize, include configure parameters */
    _ECSPI_Config(&ecspiMasterInitConfig);

    xferSemaphore = xSemaphoreCreateBinary();
    if (!xferSemaphore) {
        return 1;
    }

    return 0;
}


/* Fill the TXFIFO. */
static bool _ECSPI_TransmitBurst() {
    uint8_t bytes;
    uint32_t data;
    uint8_t i;

    /* Fill the TXFIFO */
    while(
        (ecspiState.txSize > 0) &&
        (ECSPI_GetStatusFlag(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagTxfifoFull) == 0)
    ) {
        bytes = ecspiState.txSize & 0x3;      /* first get unaligned part transmitted */
        bytes = bytes ? bytes : 4;             /* if aligned, then must be 4 */

        if(!(ecspiState.txBuffPtr)) {
            data = 0xFFFFFFFF;                 /* half-duplex receive data */
        } else {
            data = 0;
            for(i = 0; i < bytes; i++) {
                data = (data << 8) | *(ecspiState.txBuffPtr)++;
            }
        }

        ECSPI_SendData(BOARD_ECSPI_MASTER_BASEADDR, data);
        ecspiState.txSize -= bytes;
        ecspiState.rxSize += bytes;
    }
    /* start transmission */
    ECSPI_StartBurst(BOARD_ECSPI_MASTER_BASEADDR);
    /* set transfer flag */
    ecspiState.isBusy = true;

    return true;
}


/* Receive data from RXFIFO */
static bool _ECSPI_ReceiveBurst() {
    uint32_t data;
    uint32_t bytes;
    uint32_t i;

    while (
        (ecspiState.rxSize > 0) &&
        (ECSPI_GetStatusFlag(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagRxfifoReady) != 0)
    ) {
        data = ECSPI_ReceiveData(BOARD_ECSPI_MASTER_BASEADDR);   /* read data from register */
        bytes = ecspiState.rxSize & 0x3;                  /* first get unaligned part received */
        bytes = bytes ? bytes : 4;                        /* if aligned, then must be 4 */

        if(ecspiState.rxBuffPtr) {                        /* not half-duplex transmit */
            for(i = bytes; i > 0; i--) {
                *(ecspiState.rxBuffPtr + i - 1) = data & 0xFF;
                data >>= 8;
            }
            ecspiState.rxBuffPtr += bytes;
        }
        ecspiState.rxSize -= bytes;
    }
    return true;
}

/* Transmit and Receive an amount of data in no-blocking mode with interrupt. */
uint8_t APP_ECSPI_Transfer(uint8_t* txBuffer, uint8_t* rxBuffer, uint32_t transferSize, uint32_t timeout) {
    uint32_t len;

    if((ecspiState.isBusy) || (transferSize == 0)) {
        return false;
    }

    /* Update the burst length to real size */
    len = (uint32_t)(transferSize * 8 - 1);
    ECSPI_SetBurstLength(BOARD_ECSPI_MASTER_BASEADDR, len);

    /* Configure the transfer */
    ecspiState.txBuffPtr = txBuffer;
    ecspiState.rxBuffPtr = rxBuffer;
    ecspiState.txSize = transferSize;
    ecspiState.rxSize = 0;

    /* Fill the TXFIFO */
    _ECSPI_TransmitBurst();
    /* Enable interrupts */
    ECSPI_SetIntCmd(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagTxfifoEmpty, true);

    /* Wait for transfer to complete */
    if (xSemaphoreTake(xferSemaphore, ms_to_ticks(timeout)) != pdTRUE) {
        /* FIXME: Recover from timeout. */
        APP_ERROR("ECSPI transfer timed out");
        return 1;
    }

    return 0;
}

/* Get transfer status. */
/*
static bool _ECSPI_GetTransferStatus() {
    return ecspiState.isBusy;
}
*/

/* ECSPI module initialize */
static void _ECSPI_Config(ecspi_init_config_t* initConfig) {
    /* Initialize ECSPI transfer state. */
    ecspiState.isBusy = false;

    /* Initialize ECSPI, parameter configure */
    ECSPI_Init(BOARD_ECSPI_MASTER_BASEADDR, initConfig);

    /* Call core API to enable the IRQ. */
    NVIC_EnableIRQ(BOARD_ECSPI_MASTER_IRQ_NUM);
}

/* The interrupt service routine triggered by ECSPI interrupt */
void BOARD_ECSPI_MASTER_HANDLER() {
    BaseType_t txHptw = pdFALSE;

    /* Receive data from RXFIFO */
    _ECSPI_ReceiveBurst();

    /* Push data left */
    if(ecspiState.txSize) {
        _ECSPI_TransmitBurst();
        return;
    }

    /* No data left to push, but still waiting for rx data, enable receive data available interrupt. */
    if(ecspiState.rxSize) {
        ECSPI_SetIntCmd(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagRxfifoReady, true);
        return;
    }

    /* Disable interrupt */
    ECSPI_SetIntCmd(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagTxfifoEmpty, false);
    ECSPI_SetIntCmd(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagRxfifoReady, false);

    /* Clear the status */
    ECSPI_ClearStatusFlag(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagTxfifoTc);
    ECSPI_ClearStatusFlag(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagRxfifoOverflow);

    ecspiState.isBusy = false;
    xSemaphoreGiveFromISR(xferSemaphore, &txHptw);

    /* Yield to higher priority task */
    portYIELD_FROM_ISR(txHptw);
}
