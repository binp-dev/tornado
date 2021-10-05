#include "skifio.h"

#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <hal/assert.h>
#include <hal/spi.h>

#include <crc.h>

#define XFER_LEN 26
#define XFER_LEN_4 ((XFER_LEN - 1) / 4 + 1)

hal_retcode skifio_init() {
    hal_spi_init();
    hal_retcode st = hal_spi_enable(0, 10000000, HAL_SPI_PHASE_SECOND_EDGE, HAL_SPI_POLARITY_ACTIVE_HIGH);
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
    // i.MX8 SPI buffers must be 4-byte aligned
    uint32_t tx4[XFER_LEN_4] = {0}; 
    uint32_t rx4[XFER_LEN_4] = {0};
    uint8_t * const tx = (uint8_t *)tx4;
    uint8_t * const rx = (uint8_t *)rx4;
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
    st = hal_spi_xfer(0, tx4, rx4, XFER_LEN, HAL_WAIT_FOREVER);
    if (st != HAL_SUCCESS) {
        return st;
    }

    // Load ADC values
    const size_t in_data_len = SKIFIO_ADC_CHANNEL_COUNT * 4;
    memcpy(in->adc, rx, in_data_len);

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
