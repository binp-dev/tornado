/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hal/assert.h>
#include <hal/spi.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_ecspi.h"
#include "fsl_ecspi_freertos.h"

#define HAL_SPI_IRQ_PRIORITY 3

#define ECSPI_TRANSFER_SIZE     64
#define ECSPI_MASTER_BASEADDR   ECSPI1
#define ECSPI_MASTER_CLK_FREQ                                                                 \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootEcspi1)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootEcspi1)))
#define ECSPI_MASTER_TRANSFER_CHANNEL kECSPI_Channel0
#define ECSPI_MASTER_IRQN     ECSPI1_IRQn

static ecspi_master_config_t masterConfig;
static ecspi_transfer_t masterXfer;

static ecspi_rtos_handle_t master_rtos_handle;


void hal_spi_init() {
    CLOCK_SetRootMux(kCLOCK_RootEcspi1, kCLOCK_EcspiRootmuxSysPll1); /* Set ECSPI1 source to SYSTEM PLL1 800MHZ */
    CLOCK_SetRootDivider(kCLOCK_RootEcspi1, 2U, 5U);                 /* Set root clock to 800MHZ / 10 = 80MHZ */
}

void hal_spi_deinit() {
    /// FIXME: Implement SPI deinitialization.
}

hal_retcode hal_spi_enable(uint32_t channel, uint32_t baud_rate, HalSpiPhase phase, HalSpiPolarity polarity) {
    /// FIXME: Use all available controllers.
    if (channel != 0) {
        return HAL_OUT_OF_BOUNDS;
    }

    NVIC_SetPriority(ECSPI_MASTER_IRQN, HAL_SPI_IRQ_PRIORITY);

    ecspi_clock_polarity_t clock_polarity;
    switch (polarity) {
    case HAL_SPI_POLARITY_ACTIVE_HIGH:
        clock_polarity = kECSPI_PolarityActiveHigh;
        break;
    case HAL_SPI_POLARITY_ACTIVE_LOW:
        clock_polarity = kECSPI_PolarityActiveLow;
        break;
    default:
        hal_unreachable();
    }

    ecspi_clock_phase_t clock_phase;
    switch (phase) {
    case HAL_SPI_PHASE_FIRST_EDGE:
        clock_phase = kECSPI_ClockPhaseFirstEdge;
        break;
    case HAL_SPI_PHASE_SECOND_EDGE:
        clock_phase = kECSPI_ClockPhaseSecondEdge;
        break;
    default:
        hal_unreachable();
    }

    ECSPI_MasterGetDefaultConfig(&masterConfig);
    masterConfig.channelConfig.phase = clock_phase;
    masterConfig.channelConfig.polarity = clock_polarity;
    masterConfig.baudRate_Bps = baud_rate;

    status_t status = ECSPI_RTOS_Init(&master_rtos_handle, ECSPI_MASTER_BASEADDR, &masterConfig, ECSPI_MASTER_CLK_FREQ);
    if (status != kStatus_Success)
    {
        return HAL_FAILURE;
    }

    return HAL_SUCCESS;
}

hal_retcode hal_spi_disable(uint32_t channel) {
    /// FIXME: Use all available controllers.
    if (channel != 0) {
        return HAL_OUT_OF_BOUNDS;
    }
    /* Deinit the ECSPI. */
    ECSPI_RTOS_Deinit(&master_rtos_handle);
    return HAL_SUCCESS;
}

hal_retcode hal_spi_xfer(uint32_t channel, uint32_t *tx_buf, uint32_t *rx_buf, size_t len, uint32_t timeout) {
    /// FIXME: Use all available controllers.
    if (channel != 0) {
        return HAL_OUT_OF_BOUNDS;
    }
    /// FIXME: Support timeout
    if (timeout != HAL_WAIT_FOREVER) {
        return HAL_UNIMPLEMENTED;
    }

    masterXfer.txData   = tx_buf;
    masterXfer.rxData   = rx_buf;
    masterXfer.dataSize = len;
    masterXfer.channel  = ECSPI_MASTER_TRANSFER_CHANNEL;

    /*Start master transfer*/
    status_t status = ECSPI_RTOS_Transfer(&master_rtos_handle, &masterXfer);
    if (status != kStatus_Success)
    {
        return HAL_FAILURE;
    }
    return HAL_SUCCESS;
}
