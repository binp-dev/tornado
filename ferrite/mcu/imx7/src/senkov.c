#include "senkov.h"

#include <FreeRTOS.h>
#include <task.h>

#include <hal/assert.h>
#include <hal/spi.h>

#define PREFIX_SHIFT 24
#define XFER_LEN 4

hal_retcode senkov_init() {
    hal_assert(sizeof(SenkovControlReg) == 1);
    hal_spi_init();
    // FIXME: We use 10 Mbps because response bits are shifted when using 25 Mbps.
    //        Maybe baud rate in HAL should be multiplied by some factor?
    hal_retcode st = hal_spi_enable(0, 10000000, HAL_SPI_PHASE_SECOND_EDGE, HAL_SPI_POLARITY_ACTIVE_HIGH);
    if (st != HAL_SUCCESS) {
        hal_spi_deinit();
        return st;
    }
    return HAL_SUCCESS;
}

hal_retcode senkov_deinit() {
    hal_retcode st = hal_spi_disable(0);
    if (st != HAL_SUCCESS) {
        return st;
    }
    hal_spi_deinit();
    return HAL_SUCCESS;
}

static hal_retcode make_request(uint32_t request, uint32_t *response) {
    hal_retcode st;
    uint8_t tx[XFER_LEN];
    uint8_t rx[XFER_LEN] = {0};
    for (size_t i = 0; i < XFER_LEN; ++i) {
        tx[i] = (uint8_t)(request >> (8 * (XFER_LEN - i - 1)));
    }

    st = hal_spi_xfer(0, tx, rx, XFER_LEN, HAL_WAIT_FOREVER);
    if (st != HAL_SUCCESS) {
        return st;
    }
    vTaskDelay(1);

    for (size_t i = 0; i < XFER_LEN; ++i) {
        tx[i] = (uint8_t)0;
        rx[i] = (uint8_t)0;
    }
    st = hal_spi_xfer(0, tx, rx, XFER_LEN, HAL_WAIT_FOREVER);
    if (st != HAL_SUCCESS) {
        return st;
    }

    *response = 0;
    for (size_t i = 0; i < XFER_LEN; ++i) {
        *response |= (uint32_t)rx[i] << (8 * (XFER_LEN - i - 1));
    }
    return HAL_SUCCESS;
}

hal_retcode senkov_read_adc(uint32_t index, uint32_t *value) {
    if (index > 6) {
        return HAL_INVALID_INPUT;
    }
    uint32_t rx = 0;
    uint32_t tx = 0xC0000000 | index;
    hal_retcode st;
    st = make_request(tx, &rx);
    if (st != HAL_SUCCESS) {
        return st;
    }
    if ((rx >> PREFIX_SHIFT) != (0xC0 | index)) {
        return HAL_INVALID_DATA;
    }

    *value = rx & 0x00FFFFFF;
    return HAL_SUCCESS;
}

static hal_retcode send_value(uint8_t prefix, uint32_t value) {
    if ((value >> PREFIX_SHIFT) != 0) {
        return HAL_INVALID_INPUT;
    }
    uint32_t tx = ((uint32_t)prefix << PREFIX_SHIFT) | value;
    uint32_t rx = 0;
    hal_retcode st;
    st = make_request(tx, &rx);
    if (st != HAL_SUCCESS) {
        return st;
    }
    if (tx != rx) {
        return HAL_INVALID_DATA;
    }

    return HAL_SUCCESS;
}

hal_retcode senkov_write_dac(uint32_t value) {
    return send_value(0xE0, value);
}

hal_retcode senkov_write_control_reg(SenkovControlReg reg) {
    uint32_t value = (uint32_t)*(uint8_t*)&reg;
    return send_value(0xE1, value);
}

hal_retcode senkov_write_set_period(uint32_t value) {
    return send_value(0xE2, value);
}

hal_retcode senkov_write_send_period(uint32_t value) {
    return send_value(0xE3, value);
}
