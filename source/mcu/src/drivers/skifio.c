#include "skifio.h"

#include <string.h>
#include <stdio.h>

#include <fsl_common.h>
#include <fsl_iomuxc.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <hal/assert.h>
#include <hal/spi.h>
#include <hal/gpio.h>
#include <hal/time.h>

#include <utils/crc.h>


#define SPI_BAUD_RATE 25000000

#define FIRST_SAMPLES_TO_SKIP 1
#define READY_DELAY_NS 0

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

#define DIN_0_MUX IOMUXC_GPIO1_IO01_GPIO1_IO01
#define DIN_0_PIN 1, 1
#define DIN_1_MUX IOMUXC_GPIO1_IO11_GPIO1_IO11
#define DIN_1_PIN 1, 11
#define DIN_2_MUX IOMUXC_GPIO1_IO13_GPIO1_IO13
#define DIN_2_PIN 1, 13
#define DIN_3_MUX IOMUXC_GPIO1_IO15_GPIO1_IO15
#define DIN_3_PIN 1, 15
#define DIN_4_MUX IOMUXC_SPDIF_RX_GPIO5_IO04
#define DIN_4_PIN 5, 4
#define DIN_5_MUX IOMUXC_SPDIF_EXT_CLK_GPIO5_IO05
#define DIN_5_PIN 5, 5
#define DIN_6_MUX IOMUXC_I2C4_SCL_GPIO5_IO20
#define DIN_6_PIN 5, 20
#define DIN_7_MUX IOMUXC_I2C4_SDA_GPIO5_IO21
#define DIN_7_PIN 5, 21

#define DOUT_0_MUX IOMUXC_SAI2_RXD0_GPIO4_IO23
#define DOUT_0_PIN 4, 23
#define DOUT_1_MUX IOMUXC_SAI2_TXD0_GPIO4_IO26
#define DOUT_1_PIN 4, 26
#define DOUT_2_MUX IOMUXC_SAI2_MCLK_GPIO4_IO27
#define DOUT_2_PIN 4, 27
#define DOUT_3_MUX IOMUXC_SAI3_RXC_GPIO4_IO29
#define DOUT_3_PIN 4, 29

typedef struct {
    uint32_t mux[5];
    HalGpioBlockIndex block;
    HalGpioPinIndex index;
    bool intr;
} PinInfo;

static const PinInfo DIN_PINS[SKIFIO_DIN_SIZE] = {
    {{DIN_0_MUX}, DIN_0_PIN, false},
    {{DIN_1_MUX}, DIN_1_PIN, false},
    {{DIN_2_MUX}, DIN_2_PIN, false},
    {{DIN_3_MUX}, DIN_3_PIN, false},
    {{DIN_4_MUX}, DIN_4_PIN, false},
    {{DIN_5_MUX}, DIN_5_PIN, false},
    {{DIN_6_MUX}, DIN_6_PIN, false},
    {{DIN_7_MUX}, DIN_7_PIN, false},
};

static const PinInfo DOUT_PINS[SKIFIO_DOUT_SIZE] = {
    {{DOUT_0_MUX}, DOUT_0_PIN, false},
    {{DOUT_1_MUX}, DOUT_1_PIN, false},
    {{DOUT_2_MUX}, DOUT_2_PIN, false},
    {{DOUT_3_MUX}, DOUT_3_PIN, false},
};

typedef struct {
    HalGpioGroup group;
    HalGpioPin read_rdy;
    HalGpioPin smp_rdy;
    HalGpioPin dac_keys[2];
} SkifioControlPins;

typedef struct {
    HalGpioGroup group;
    HalGpioPin din[SKIFIO_DIN_SIZE];
    HalGpioPin dout[SKIFIO_DOUT_SIZE];
} SkifioDioPins;

typedef struct {
    SkifioControlPins ctrl_pins;
    SkifioDioPins dio_pins;
    SemaphoreHandle_t smp_rdy_sem;
    volatile SkifioDinCallback din_callback;
    void *volatile din_user_data;
    volatile size_t sample_skip_counter;
} SkifioGlobalState;

static SkifioGlobalState GS;

#ifdef _SKIFIO_DEBUG
_SkifioDebugInfo _SKIFIO_DEBUG_INFO = {0};
#endif


static void smp_rdy_handler(void *user_data, HalGpioBlockIndex block, HalGpioPinMask mask) {
    BaseType_t hptw = pdFALSE;

    if (GS.sample_skip_counter == 0) {
#ifdef _SKIFIO_DEBUG
        _SKIFIO_DEBUG_INFO.intr_count += 1;
#endif
        // Notify target task
        xSemaphoreGiveFromISR(GS.smp_rdy_sem, &hptw);
    } else {
        GS.sample_skip_counter -= 1;
    }

    // Yield to higher priority task
    portYIELD_FROM_ISR(hptw);
}

static void din_handler(void *data, HalGpioBlockIndex block, HalGpioPinMask pins) {
    void *user_data = GS.din_user_data;
    SkifioDinCallback callback = GS.din_callback;
    if (callback != NULL) {
        callback(user_data, skifio_din_read());
    }
}

void init_ctrl_pins() {
    IOMUXC_SetPinMux(SMP_RDY_MUX, 0U);
    IOMUXC_SetPinMux(READ_RDY_MUX, 0U);
    IOMUXC_SetPinMux(DAC_KEY_0_MUX, 0U);
    IOMUXC_SetPinMux(DAC_KEY_1_MUX, 0U);

    hal_assert(hal_gpio_group_init(&GS.ctrl_pins.group) == HAL_SUCCESS);
    hal_assert(
        hal_gpio_pin_init(&GS.ctrl_pins.read_rdy, &GS.ctrl_pins.group, READ_RDY_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED)
        == HAL_SUCCESS);
    hal_assert(
        hal_gpio_pin_init(&GS.ctrl_pins.smp_rdy, &GS.ctrl_pins.group, SMP_RDY_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_RISING_EDGE)
        == HAL_SUCCESS);
    hal_assert(
        hal_gpio_pin_init(
            &GS.ctrl_pins.dac_keys[0],
            &GS.ctrl_pins.group,
            DAC_KEY_0_PIN,
            HAL_GPIO_OUTPUT,
            HAL_GPIO_INTR_DISABLED)
        == HAL_SUCCESS);
    hal_assert(
        hal_gpio_pin_init(
            &GS.ctrl_pins.dac_keys[1],
            &GS.ctrl_pins.group,
            DAC_KEY_1_PIN,
            HAL_GPIO_OUTPUT,
            HAL_GPIO_INTR_DISABLED)
        == HAL_SUCCESS);
    hal_gpio_pin_write(&GS.ctrl_pins.read_rdy, false);
}

void init_dio_pins() {
    hal_assert(hal_gpio_group_init(&GS.dio_pins.group) == HAL_SUCCESS);

    for (size_t i = 0; i < SKIFIO_DIN_SIZE; ++i) {
        const PinInfo *pin = &DIN_PINS[i];
        IOMUXC_SetPinMux(pin->mux[0], pin->mux[1], pin->mux[2], pin->mux[3], pin->mux[4], 0U);
        hal_assert(
            hal_gpio_pin_init(
                &GS.dio_pins.din[i],
                &GS.dio_pins.group,
                pin->block,
                pin->index,
                HAL_GPIO_INPUT,
                pin->intr ? HAL_GPIO_INTR_RISING_OR_FALLING_EDGE : HAL_GPIO_INTR_DISABLED)
            == HAL_SUCCESS);
    }

    for (size_t i = 0; i < SKIFIO_DOUT_SIZE; ++i) {
        const PinInfo *pin = &DOUT_PINS[i];
        IOMUXC_SetPinMux(pin->mux[0], pin->mux[1], pin->mux[2], pin->mux[3], pin->mux[4], 0U);
        hal_assert(
            hal_gpio_pin_init(
                &GS.dio_pins.dout[i],
                &GS.dio_pins.group,
                pin->block,
                pin->index,
                HAL_GPIO_OUTPUT,
                HAL_GPIO_INTR_DISABLED)
            == HAL_SUCCESS);
    }

    hal_assert(hal_gpio_group_set_intr(&GS.dio_pins.group, din_handler, NULL) == HAL_SUCCESS);
}

void switch_dac_keys(bool state) {
    hal_gpio_pin_write(&GS.ctrl_pins.dac_keys[0], state);
    hal_gpio_pin_write(&GS.ctrl_pins.dac_keys[1], state);
}

hal_retcode init_spi() {
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

    return HAL_SUCCESS;
}

hal_retcode skifio_init() {
    hal_retcode ret;

    GS.din_callback = NULL;
    GS.din_user_data = NULL;

    GS.sample_skip_counter = FIRST_SAMPLES_TO_SKIP;

    init_ctrl_pins();
    init_dio_pins();

    GS.smp_rdy_sem = xSemaphoreCreateBinary();
    hal_assert(GS.smp_rdy_sem != NULL);

    ret = init_spi();
    if (ret != HAL_SUCCESS) {
        return ret;
    }

#ifdef _SKIFIO_DEBUG
    _SKIFIO_DEBUG_INFO.intr_count = 0;
#endif
    ret = hal_gpio_group_set_intr(&GS.ctrl_pins.group, smp_rdy_handler, NULL);
    if (ret != HAL_SUCCESS) {
        return ret;
    }

    return HAL_SUCCESS;
}

hal_retcode skifio_deinit() {
    hal_gpio_group_set_intr(&GS.ctrl_pins.group, NULL, NULL);
    switch_dac_keys(false);

    hal_retcode st = hal_spi_disable(SPI_DEV_ID);
    if (st != HAL_SUCCESS) {
        return st;
    }
    hal_spi_deinit();
    return HAL_SUCCESS;
}

hal_retcode skifio_dac_enable() {
    switch_dac_keys(true);
    return HAL_SUCCESS;
}

hal_retcode skifio_dac_disable() {
    switch_dac_keys(false);
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
    for (size_t i = 0; i < XFER_LEN; ++i) {
        tx4[i] = (hal_spi_byte)tx[i];
    }
    st = hal_spi_xfer(SPI_DEV_ID, tx4, rx4, XFER_LEN, HAL_WAIT_FOREVER);
    if (st != HAL_SUCCESS) {
        return st;
    }
    for (size_t i = 0; i < XFER_LEN; ++i) {
        rx[i] = (uint8_t)rx4[i];
    }

    // Load ADC values
    const size_t in_data_len = SKIFIO_ADC_CHANNEL_COUNT * 4;
    memcpy(in->adcs, rx, in_data_len);

    // Load and check CRC
    calc_crc = calculate_crc16(rx, in_data_len);
    uint16_t in_crc = 0;
    memcpy(&in_crc, rx + in_data_len, 2);
    if (calc_crc != in_crc) {
        // CRC mismatch
        return HAL_INVALID_DATA;
    }

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

hal_retcode skifio_dout_write(SkifioDout value) {
    if ((value & ~((1 << SKIFIO_DOUT_SIZE) - 1)) != 0) {
        return HAL_INVALID_INPUT;
    }
    for (size_t i = 0; i < SKIFIO_DOUT_SIZE; ++i) {
        hal_gpio_pin_write(&GS.dio_pins.dout[i], (value & (1 << i)) != 0);
    }
    return HAL_SUCCESS;
}

SkifioDin skifio_din_read() {
    SkifioDin value = 0;
    for (size_t i = 0; i < SKIFIO_DIN_SIZE; ++i) {
        if (hal_gpio_pin_read(&GS.dio_pins.din[i])) {
            value |= (SkifioDin)(1 << i);
        }
    }
    return value;
}

hal_retcode skifio_din_subscribe(SkifioDinCallback callback, void *data) {
    if (GS.din_callback != NULL) {
        return HAL_FAILURE;
    }
    GS.din_callback = callback;
    GS.din_user_data = data;
    return HAL_SUCCESS;
}

hal_retcode skifio_din_unsubscribe() {
    GS.din_callback = NULL;
    GS.din_user_data = NULL;
    return HAL_SUCCESS;
}
