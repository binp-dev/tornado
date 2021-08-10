#include "senkov.h"

#include <hal/spi.h>

hal_retcode senkov_init() {
    hal_assert(sizeof(SenkovControlReg) == 1);
    hal_spi_init();
    hal_retcode st = hal_spi_enable(0, 1000000);
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
}

static hal_retcode make_request(uint32_t request, uint32_t *response) {
    hal_retcode st;
    uint32_t zeros = 0;
    st = hal_spi_xfer(0, (uint8_t*)&request, (uint8_t*)&zeros, 4, HAL_WAIT_FOREVER);
    if (st != HAL_SUCCESS) {
        return st;
    }

    zeros = 0;
    st = hal_spi_xfer(0, (uint8_t*)&zeros, (uint8_t*)response, 4, HAL_WAIT_FOREVER);
    if (st != HAL_SUCCESS) {
        return st;
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
    if ((rx >> 24) != (0xC0 | index)) {
        return HAL_INVALID_DATA;
    }

    *value = rx & 0x00FFFFFF;
    return HAL_SUCCESS;
}

static hal_retcode send_value(uint8_t prefix, uint32_t value) {
    if ((value >> 24) != 0) {
        return HAL_INVALID_INPUT;
    }
    uint32_t tx = ((uint32_t)prefix << 24) | value;
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

hal_retcode senkov_write_control_reg(SenkovControlReg value) {
    uint32_t value = (uint32_t)*(uint8_t*)&value
    return send_value(0xE1, value);
}

hal_retcode senkov_write_set_period(uint32_t value) {
    return send_value(0xE2, value);
}

hal_retcode senkov_write_send_period(uint32_t value) {
    return send_value(0xE3, value);
}
