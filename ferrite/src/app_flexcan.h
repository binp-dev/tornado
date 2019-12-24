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

/*!
 * @file app_flexcan.h
 * @brief Application-specific FLEXCAN abstraction layer
 */

#pragma once

#include <stdint.h>
#include "flexcan.h"


/*! @brief Available CAN bus baudrates */
typedef enum {
    APP_FLEXCAN_Baudrate_1000 = 3, /*!< Baudrate 1 MHz */
    APP_FLEXCAN_Baudrate_500  = 2, /*!< Baudrate 500 kHz */
    APP_FLEXCAN_Baudrate_250  = 1, /*!< Baudrate 250 kHz */
    APP_FLEXCAN_Baudrate_125  = 0  /*!< Baudrate 125 kHz */
} APP_FLEXCAN_Baudrate;

/*! @brief CAN frame structure. */
typedef struct {
    uint16_t id;     /*!< Frame ID. */
    uint8_t len;     /*!< Frame payload length in bytes (max 8 bytes). */
    uint8_t data[8]; /*!< Frame payload (first byte is last in frame). */
} APP_FLEXCAN_Frame;

/*! @brief Initialize FLEXCAN hardware, usually called from `hardware_init()`. */
void APP_FLEXCAN_HardwareInit();

/*!
 * @brief Configure CAN subsystem.
 *
 * @param rate_id CAN baudrate enum.
 * @param rx_mask Incoming frame ID mask.
 * @return Status, zero on success.
 */
uint8_t APP_FLEXCAN_Init(APP_FLEXCAN_Baudrate rate_id, uint16_t rx_mask);

/*!
 * @brief Send CAN frame synchronously.
 *
 * @param frame Read-only CAN frame structure to send.
 * @return Operation status, zero on success.
 */
uint8_t APP_FLEXCAN_Send(const APP_FLEXCAN_Frame *frame);

/*!
 * @brief Get CAN frame if it has been received.
 *
 * @param frame CAN frame structure to write into.
 * @return Operation status, zero on success.
 */
uint8_t APP_FLEXCAN_TryRecv(APP_FLEXCAN_Frame *frame);
