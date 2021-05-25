#include <stdint.h>
#include <stdbool.h>
#include <board.h>
#include <ecspi.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include <hal/spi.h>

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

static TickType_t ms_to_ticks(uint32_t ms) {
    if (ms == HAL_NON_BLOCK) {
        return 0;
    } else if (ms == HAL_WAIT_FOREVER) {
        return portMAX_DELAY;
    } else {
        return (ms - 1)/portTICK_PERIOD_MS + 1;
    }
}

/* ECSPI runtime state structure */
static volatile ecspi_state_t ecspiState;

static SemaphoreHandle_t xferSemaphore = NULL;

static bool ECSPI_TransmitBurst();
static bool ECSPI_ReceiveBurst();
//static bool ECSPI_GetTransferStatus();
static void ECSPI_Config(ecspi_init_config_t* initConfig);

/*! @brief ECSPI interrupt handler. */
void BOARD_ECSPI_MASTER_HANDLER();

void hal_spi_init() {
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

void hal_spi_deinit() {
    /// FIXME: Implement SPI deinitialization.
}

hal_retcode hal_spi_enable(uint32_t channel, uint32_t baud_rate) {
    /// FIXME: Use all available controllers.
    if (channel != 0) {
        return HAL_OUT_OF_BOUNDS;
    }

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
    ECSPI_Config(&ecspiMasterInitConfig);

    xferSemaphore = xSemaphoreCreateBinary();
    if (!xferSemaphore) {
        return HAL_BAD_ALLOC;
    }

    return HAL_SUCCESS;
}

hal_retcode hal_spi_disable(uint32_t channel) {
    /// FIXME: Use all available controllers.
    if (channel != 0) {
        return HAL_OUT_OF_BOUNDS;
    }
    /// FIXME: Implement.
    return HAL_UNIMPLEMENTED;
}

hal_retcode hal_spi_xfer(uint32_t channel, uint8_t* tx_buf, uint8_t* rx_buf, size_t len, uint32_t timeout) {
    /// FIXME: Use all available controllers.
    if (channel != 0) {
        return HAL_OUT_OF_BOUNDS;
    }

    uint32_t burst_len;

    if((ecspiState.isBusy) || (len == 0)) {
        return false;
    }

    /* Update the burst length to real size */
    burst_len = (uint32_t)(len * 8 - 1);
    ECSPI_SetBurstLength(BOARD_ECSPI_MASTER_BASEADDR, burst_len);

    /* Configure the transfer */
    ecspiState.txBuffPtr = tx_buf;
    ecspiState.rxBuffPtr = rx_buf;
    ecspiState.txSize = len;
    ecspiState.rxSize = 0;

    /* Fill the TXFIFO */
    ECSPI_TransmitBurst();
    /* Enable interrupts */
    ECSPI_SetIntCmd(BOARD_ECSPI_MASTER_BASEADDR, ecspiFlagTxfifoEmpty, true);

    /* Wait for transfer to complete */
    if (xSemaphoreTake(xferSemaphore, ms_to_ticks(timeout)) != pdTRUE) {
        /// FIXME: Recover from timeout.
        return HAL_TIMED_OUT;
    }

    return HAL_SUCCESS;
}

/* Fill the TXFIFO. */
static bool ECSPI_TransmitBurst() {
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
static bool ECSPI_ReceiveBurst() {
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

/* Get transfer status. */
/*
static bool ECSPI_GetTransferStatus() {
    return ecspiState.isBusy;
}
*/

/* ECSPI module initialize */
static void ECSPI_Config(ecspi_init_config_t* initConfig) {
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
    ECSPI_ReceiveBurst();

    /* Push data left */
    if(ecspiState.txSize) {
        ECSPI_TransmitBurst();
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
