#include "skifio.h"

#include <string.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>


#include <hal/assert.h>
#include <hal/spi.h>

#include <crc.h>

#define _SKIFIO_SPI_BAUD_RATE 10000000

#define XFER_LEN 26

//#define _SKIFIO_PRINT

#ifdef _SKIFIO_PRINT
#include <hal/log.h>
#endif // _SKIFIO_PRINT

hal_retcode skifio_init() {
    hal_spi_init();
    hal_retcode st = hal_spi_enable(0, _SKIFIO_SPI_BAUD_RATE, HAL_SPI_PHASE_SECOND_EDGE, HAL_SPI_POLARITY_ACTIVE_HIGH);
    if (st != HAL_SUCCESS) {
        hal_spi_deinit();
        return st;
    }
    return HAL_SUCCESS;
}

hal_retcode skifio_deinit() {
    hal_retcode st = hal_spi_disable(0);
    if (st != HAL_SUCCESS) {
        return st;
    }
    hal_spi_deinit();
    return HAL_SUCCESS;
}

hal_retcode skifio_transfer(const SkifioOutput *out, SkifioInput *in) {
    hal_retcode st = HAL_SUCCESS;
    uint8_t tx[XFER_LEN] = {0};
    uint8_t rx[XFER_LEN] = {0};
    uint16_t calc_crc = 0;

    // Store magic number
    tx[0] = 0x55;
    tx[1] = 0xAA;
    
    // Store DAC value
    memcpy(tx + 2, &out->dac, 2);

    // Store CRC
    calc_crc = calculate_crc16(tx, 4);
    memcpy(tx + 4, &calc_crc, 2);

    // Transfer data
    hal_spi_byte tx4[XFER_LEN] = {0};
    hal_spi_byte rx4[XFER_LEN] = {0};
#ifdef _SKIFIO_PRINT
    char data_buf[3 * XFER_LEN + 1] = {'\0'};
#endif // _SKIFIO_PRINT
    for (size_t i = 0; i < XFER_LEN; ++i) {
        tx4[i] = (hal_spi_byte)tx[i];
#ifdef _SKIFIO_PRINT
        snprintf(data_buf + 3 * i, 4, "%02lx ", tx4[i]);
#endif // _SKIFIO_PRINT
    }
#ifdef _SKIFIO_PRINT
    hal_log_info("Tx: %s", data_buf);
#endif // _SKIFIO_PRINT
    st = hal_spi_xfer(0, tx4, rx4, XFER_LEN, HAL_WAIT_FOREVER);
    if (st != HAL_SUCCESS) {
        return st;
    }
    for (size_t i = 0; i < XFER_LEN; ++i) {
        rx[i] = (uint8_t)rx4[i];
#ifdef _SKIFIO_PRINT
        snprintf(data_buf + 3 * i, 4, "%02lx ", rx4[i]);
#endif // _SKIFIO_PRINT
    }
#ifdef _SKIFIO_PRINT
    hal_log_info("Rx: %s", data_buf);
#endif // _SKIFIO_PRINT

    // Load ADC values
    const size_t in_data_len = SKIFIO_ADC_CHANNEL_COUNT * 4;
    memcpy(in->adcs, rx, in_data_len);

    // Load and check CRC
    calc_crc = calculate_crc16(rx, in_data_len);
    uint16_t in_crc = 0;
    memcpy(&in_crc, tx + in_data_len, 2);
    if (calc_crc != in_crc) {
        // CRC mismatch
        return HAL_INVALID_DATA;
    }

    return HAL_SUCCESS;
}
