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
 * @return Initialization status, zero on success.
 */
uint8_t APP_FLEXCAN_Init(APP_FLEXCAN_Baudrate rate_id, uint16_t rx_mask);

/*!
 * @brief Send CAN frame synchronously.
 *
 * @param frame Read-only CAN frame structure to send.
 * @param timeout Time to wait for send to complete in milliseconds.
 * @return Operation status, 0 - success, 1 - timed out.
 */
uint8_t APP_FLEXCAN_Send(const APP_FLEXCAN_Frame *frame, uint32_t timeout);

/*!
 * @brief Receive CAN frame.
 *
 * @param frame CAN frame structure to write into.
 * @param timeout Time to wait for frame to be received in milliseconds.
 * @return Operation status, 0 - success, 1 - timed out.
 */
uint8_t APP_FLEXCAN_Receive(APP_FLEXCAN_Frame *frame, uint32_t timeout);
