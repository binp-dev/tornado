#pragma once

#include <stdint.h>


void APP_ECSPI_HardwareInit();

/*!
 * @brief Configure SPI subsystem.
 *
 * @param baud_rate SPI baudrate.
 * @return Status, 0 - success, 1 - timed out.
 */
uint8_t APP_ECSPI_Init(uint32_t baud_rate);

/*!
 * @brief Transfer data over SPI synchronously.
 *
 * @param txBuffer Data to transmit.
 * @param rxBuffer Where data to be placed when received.
 * @param transferSize Time to wait for send to complete in milliseconds, 0 to wait forever.
 * @param timeout Time to wait for send to complete in milliseconds, 0 to wait forever.
 * @return Operation status, zero on success.
 */
uint8_t APP_ECSPI_Transfer(uint8_t* txBuffer, uint8_t* rxBuffer, uint32_t transferSize, uint32_t timeout);
