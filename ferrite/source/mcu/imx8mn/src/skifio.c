#include "skifio.h"

#include <string.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "fsl_common.h"
#include "fsl_iomuxc.h"

#include <hal/assert.h>
#include <hal/spi.h>
#include <hal/gpio.h>
#include <hal/time.h>

#include <crc.h>

//#define _SKIFIO_PRINT_SPI

#ifdef _SKIFIO_PRINT_SPI
#include <hal/log.h>
#endif // _SKIFIO_PRINT_SPI

#define SPI_BAUD_RATE 25000000

#define READY_DELAY_NS 0
#define READ_RDY_DURATION_NS 1000

#define SPI_DEV_ID 0
#define XFER_LEN 26

#define SMP_RDY_MUX IOMUXC_UART1_TXD_GPIO5_IO23
#define SMP_RDY_PIN 5, 23

#define READ_RDY_MUX IOMUXC_ECSPI1_SS0_GPIO5_IO09
#define READ_RDY_PIN 5, 9

#define DAC_KEY_0_MUX IOMUXC_SAI3_MCLK_GPIO5_IO02
#define DAC_KEY_0_PIN 5, 2
#define DAC_KEY_1_MUX IOMUXC_SPDIF_TX_GPIO5_IO03
#define DAC_KEY_1_PIN 5, 3

typedef struct {
    HalGpioGroup group;
    HalGpioPin read_rdy;
    HalGpioPin smp_rdy;
    HalGpioPin dac_keys[2];
} SkifioPins;

typedef struct {
    SkifioPins pins;
    volatile SemaphoreHandle_t smp_rdy_sem;
} SkifioGlobalState;

static SkifioGlobalState GS;

#ifdef _SKIFIO_DEBUG
_SkifioDebugInfo _SKIFIO_DEBUG_INFO;
#endif


static void smp_rdy_handler(void *user_data, HalGpioBlockIndex block, HalGpioPinMask mask) {
#ifdef _SKIFIO_DEBUG
    _SKIFIO_DEBUG_INFO.intr_count += 1;
#endif
    BaseType_t hptw = pdFALSE;

    // Notify target task
    xSemaphoreGiveFromISR(GS.smp_rdy_sem, &hptw);

    // Yield to higher priority task
    portYIELD_FROM_ISR(hptw);
}

void cfg_gpio_pin_mux() {
    IOMUXC_SetPinMux(SMP_RDY_MUX, 0U);
    IOMUXC_SetPinMux(READ_RDY_MUX, 0U);

    IOMUXC_SetPinMux(DAC_KEY_0_MUX, 0U);
    IOMUXC_SetPinMux(DAC_KEY_1_MUX, 0U);
}

void cfg_spi_pin_mux() {
    IOMUXC_SetPinMux(IOMUXC_ECSPI1_MISO_ECSPI1_MISO, 0U);
    IOMUXC_SetPinConfig(
        IOMUXC_ECSPI1_MISO_ECSPI1_MISO,
        IOMUXC_SW_PAD_CTL_PAD_DSE(6U) | IOMUXC_SW_PAD_CTL_PAD_HYS_MASK //
    );
    IOMUXC_SetPinMux(IOMUXC_ECSPI1_MOSI_ECSPI1_MOSI, 0U);
    IOMUXC_SetPinConfig(
        IOMUXC_ECSPI1_MOSI_ECSPI1_MOSI,
        IOMUXC_SW_PAD_CTL_PAD_DSE(6U) | IOMUXC_SW_PAD_CTL_PAD_HYS_MASK //
    );
    IOMUXC_SetPinMux(IOMUXC_ECSPI1_SCLK_ECSPI1_SCLK, 0U);
    IOMUXC_SetPinConfig(
        IOMUXC_ECSPI1_SCLK_ECSPI1_SCLK,
        IOMUXC_SW_PAD_CTL_PAD_DSE(6U) | IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PE_MASK //
    );
}

void cfg_pin_mux() {
    cfg_gpio_pin_mux();
    cfg_spi_pin_mux();
}

void switch_dac_keys(bool state) {
    hal_gpio_pin_write(&GS.pins.dac_keys[0], state);
    hal_gpio_pin_write(&GS.pins.dac_keys[1], state);
}

hal_retcode skifio_init() {
    cfg_pin_mux();

    hal_gpio_group_init(&GS.pins.group);
    hal_gpio_pin_init(&GS.pins.read_rdy, &GS.pins.group, READ_RDY_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&GS.pins.smp_rdy, &GS.pins.group, SMP_RDY_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_RISING_EDGE);
    hal_gpio_pin_init(&GS.pins.dac_keys[0], &GS.pins.group, DAC_KEY_0_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&GS.pins.dac_keys[1], &GS.pins.group, DAC_KEY_1_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_write(&GS.pins.read_rdy, false);

    GS.smp_rdy_sem = xSemaphoreCreateBinary();
    hal_assert(GS.smp_rdy_sem != NULL);

    hal_spi_init();
    hal_retcode st = hal_spi_enable(
        SPI_DEV_ID,
        SPI_BAUD_RATE,
        HAL_SPI_PHASE_SECOND_EDGE,
        HAL_SPI_POLARITY_ACTIVE_HIGH //
    );
    if (st != HAL_SUCCESS) {
        hal_spi_deinit();
        return st;
    }

#ifdef _SKIFIO_DEBUG
    _SKIFIO_DEBUG_INFO.intr_count = 0;
#endif
    switch_dac_keys(true);
    hal_gpio_group_set_intr(&GS.pins.group, smp_rdy_handler, NULL);

    return HAL_SUCCESS;
}

hal_retcode skifio_deinit() {
    hal_gpio_group_set_intr(&GS.pins.group, NULL, NULL);
    switch_dac_keys(false);

    hal_retcode st = hal_spi_disable(SPI_DEV_ID);
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
#ifdef _SKIFIO_PRINT_SPI
    char data_buf[3 * XFER_LEN + 1] = {'\0'};
#endif // _SKIFIO_PRINT_SPI
    for (size_t i = 0; i < XFER_LEN; ++i) {
        tx4[i] = (hal_spi_byte)tx[i];
#ifdef _SKIFIO_PRINT_SPI
        snprintf(data_buf + 3 * i, 4, "%02lx ", tx4[i]);
#endif // _SKIFIO_PRINT_SPI
    }
#ifdef _SKIFIO_PRINT_SPI
    hal_log_info("Tx: %s", data_buf);
#endif // _SKIFIO_PRINT_SPI
    st = hal_spi_xfer(SPI_DEV_ID, tx4, rx4, XFER_LEN, HAL_WAIT_FOREVER);
    if (st != HAL_SUCCESS) {
        return st;
    }
    for (size_t i = 0; i < XFER_LEN; ++i) {
        rx[i] = (uint8_t)rx4[i];
#ifdef _SKIFIO_PRINT_SPI
        snprintf(data_buf + 3 * i, 4, "%02lx ", rx4[i]);
#endif // _SKIFIO_PRINT_SPI
    }
#ifdef _SKIFIO_PRINT_SPI
    hal_log_info("Rx: %s", data_buf);
#endif // _SKIFIO_PRINT_SPI

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

    // Notify that data is ready
    hal_gpio_pin_write(&GS.pins.read_rdy, true);
    hal_busy_wait_ns(READ_RDY_DURATION_NS);
    hal_gpio_pin_write(&GS.pins.read_rdy, false);

    return HAL_SUCCESS;
}

hal_retcode skifio_wait_ready(uint32_t timeout_ms) {
    // Wait for sample ready semaphore
    if (xSemaphoreTake(GS.smp_rdy_sem, timeout_ms) != pdTRUE) {
        return HAL_TIMED_OUT;
    }

    // Wait before data request to reduce ADC noise.
    hal_busy_wait_ns(READY_DELAY_NS);

    return HAL_SUCCESS;
}
