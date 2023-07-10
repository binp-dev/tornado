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
#define XFER_LEN 28

#define SMP_RDY_MUX IOMUXC_UART1_TXD_GPIO5_IO23
#define SMP_RDY_PIN 5, 23

#define READ_RDY_MUX IOMUXC_ECSPI1_SS0_GPIO5_IO09
#define READ_RDY_PIN 5, 9

#define AO_KEY_0_MUX IOMUXC_SAI3_MCLK_GPIO5_IO02
#define AO_KEY_0_PIN 5, 2
#define AO_KEY_1_MUX IOMUXC_SPDIF_TX_GPIO5_IO03
#define AO_KEY_1_PIN 5, 3

#define DI_0_MUX IOMUXC_GPIO1_IO01_GPIO1_IO01
#define DI_0_PIN 1, 1
#define DI_1_MUX IOMUXC_GPIO1_IO11_GPIO1_IO11
#define DI_1_PIN 1, 11
#define DI_2_MUX IOMUXC_GPIO1_IO13_GPIO1_IO13
#define DI_2_PIN 1, 13
#define DI_3_MUX IOMUXC_GPIO1_IO15_GPIO1_IO15
#define DI_3_PIN 1, 15
#define DI_4_MUX IOMUXC_SPDIF_RX_GPIO5_IO04
#define DI_4_PIN 5, 4
#define DI_5_MUX IOMUXC_SPDIF_EXT_CLK_GPIO5_IO05
#define DI_5_PIN 5, 5
#define DI_6_MUX IOMUXC_I2C4_SCL_GPIO5_IO20
#define DI_6_PIN 5, 20
#define DI_7_MUX IOMUXC_I2C4_SDA_GPIO5_IO21
#define DI_7_PIN 5, 21

#define DO_0_MUX IOMUXC_SAI2_RXD0_GPIO4_IO23
#define DO_0_PIN 4, 23
#define DO_1_MUX IOMUXC_SAI2_TXD0_GPIO4_IO26
#define DO_1_PIN 4, 26
#define DO_2_MUX IOMUXC_SAI2_MCLK_GPIO4_IO27
#define DO_2_PIN 4, 27
#define DO_3_MUX IOMUXC_SAI3_RXC_GPIO4_IO29
#define DO_3_PIN 4, 29

typedef struct {
    uint32_t mux[5];
    HalGpioBlockIndex block;
    HalGpioPinIndex index;
    bool intr;
} PinInfo;

static const PinInfo DI_PINS[SKIFIO_DI_SIZE] = {
    {{DI_0_MUX}, DI_0_PIN, false},
    {{DI_1_MUX}, DI_1_PIN, false},
    {{DI_2_MUX}, DI_2_PIN, false},
    {{DI_3_MUX}, DI_3_PIN, false},
    {{DI_4_MUX}, DI_4_PIN, false},
    {{DI_5_MUX}, DI_5_PIN, false},
    {{DI_6_MUX}, DI_6_PIN, false},
    {{DI_7_MUX}, DI_7_PIN, false},
};

static const PinInfo DO_PINS[SKIFIO_DO_SIZE] = {
    {{DO_0_MUX}, DO_0_PIN, false},
    {{DO_1_MUX}, DO_1_PIN, false},
    {{DO_2_MUX}, DO_2_PIN, false},
    {{DO_3_MUX}, DO_3_PIN, false},
};

typedef struct {
    HalGpioGroup group;
    HalGpioPin read_rdy;
    HalGpioPin smp_rdy;
    HalGpioPin ao_keys[2];
} SkifioControlPins;

typedef struct {
    HalGpioGroup group;
    HalGpioPin di[SKIFIO_DI_SIZE];
    HalGpioPin do_[SKIFIO_DO_SIZE];
} SkifioDioPins;

typedef struct {
    SkifioControlPins ctrl_pins;
    SkifioDioPins dio_pins;
    SemaphoreHandle_t smp_rdy_sem;
    volatile SkifioDiCallback di_callback;
    void *volatile di_user_data;
    volatile size_t sample_skip_counter;
} SkifioGlobalState;

static SkifioGlobalState GS;

extern void user_sample_intr();

static void smp_rdy_handler(void *user_data, HalGpioBlockIndex block, HalGpioPinMask mask) {
    BaseType_t hptw = pdFALSE;

    if (GS.sample_skip_counter == 0) {
        user_sample_intr();
        // Notify target task
        xSemaphoreGiveFromISR(GS.smp_rdy_sem, &hptw);
    } else {
        GS.sample_skip_counter -= 1;
    }

    // Yield to higher priority task
    portYIELD_FROM_ISR(hptw);
}

static void di_handler(void *data, HalGpioBlockIndex block, HalGpioPinMask pins) {
    void *user_data = GS.di_user_data;
    SkifioDiCallback callback = GS.di_callback;
    if (callback != NULL) {
        callback(user_data, skifio_di_read());
    }
}

void init_ctrl_pins() {
    IOMUXC_SetPinMux(SMP_RDY_MUX, 0U);
    IOMUXC_SetPinMux(READ_RDY_MUX, 0U);
    IOMUXC_SetPinMux(AO_KEY_0_MUX, 0U);
    IOMUXC_SetPinMux(AO_KEY_1_MUX, 0U);

    hal_assert(hal_gpio_group_init(&GS.ctrl_pins.group) == HAL_SUCCESS);
    hal_assert(
        hal_gpio_pin_init(&GS.ctrl_pins.read_rdy, &GS.ctrl_pins.group, READ_RDY_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED)
        == HAL_SUCCESS);
    hal_assert(
        hal_gpio_pin_init(&GS.ctrl_pins.smp_rdy, &GS.ctrl_pins.group, SMP_RDY_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_RISING_EDGE)
        == HAL_SUCCESS);
    hal_assert(
        hal_gpio_pin_init(
            &GS.ctrl_pins.ao_keys[0],
            &GS.ctrl_pins.group,
            AO_KEY_0_PIN,
            HAL_GPIO_OUTPUT,
            HAL_GPIO_INTR_DISABLED)
        == HAL_SUCCESS);
    hal_assert(
        hal_gpio_pin_init(
            &GS.ctrl_pins.ao_keys[1],
            &GS.ctrl_pins.group,
            AO_KEY_1_PIN,
            HAL_GPIO_OUTPUT,
            HAL_GPIO_INTR_DISABLED)
        == HAL_SUCCESS);
    hal_gpio_pin_write(&GS.ctrl_pins.read_rdy, false);
}

void init_dio_pins() {
    hal_assert(hal_gpio_group_init(&GS.dio_pins.group) == HAL_SUCCESS);

    for (size_t i = 0; i < SKIFIO_DI_SIZE; ++i) {
        const PinInfo *pin = &DI_PINS[i];
        IOMUXC_SetPinMux(pin->mux[0], pin->mux[1], pin->mux[2], pin->mux[3], pin->mux[4], 0U);
        hal_assert(
            hal_gpio_pin_init(
                &GS.dio_pins.di[i],
                &GS.dio_pins.group,
                pin->block,
                pin->index,
                HAL_GPIO_INPUT,
                pin->intr ? HAL_GPIO_INTR_RISING_OR_FALLING_EDGE : HAL_GPIO_INTR_DISABLED)
            == HAL_SUCCESS);
    }

    for (size_t i = 0; i < SKIFIO_DO_SIZE; ++i) {
        const PinInfo *pin = &DO_PINS[i];
        IOMUXC_SetPinMux(pin->mux[0], pin->mux[1], pin->mux[2], pin->mux[3], pin->mux[4], 0U);
        hal_assert(
            hal_gpio_pin_init(
                &GS.dio_pins.do_[i],
                &GS.dio_pins.group,
                pin->block,
                pin->index,
                HAL_GPIO_OUTPUT,
                HAL_GPIO_INTR_DISABLED)
            == HAL_SUCCESS);
    }

    hal_assert(hal_gpio_group_set_intr(&GS.dio_pins.group, di_handler, NULL) == HAL_SUCCESS);
}

void switch_ao_keys(bool state) {
    hal_gpio_pin_write(&GS.ctrl_pins.ao_keys[0], state);
    hal_gpio_pin_write(&GS.ctrl_pins.ao_keys[1], state);
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

    GS.di_callback = NULL;
    GS.di_user_data = NULL;

    GS.sample_skip_counter = FIRST_SAMPLES_TO_SKIP;

    init_ctrl_pins();
    init_dio_pins();

    GS.smp_rdy_sem = xSemaphoreCreateBinary();
    hal_assert(GS.smp_rdy_sem != NULL);

    ret = init_spi();
    if (ret != HAL_SUCCESS) {
        return ret;
    }

    ret = hal_gpio_group_set_intr(&GS.ctrl_pins.group, smp_rdy_handler, NULL);
    if (ret != HAL_SUCCESS) {
        return ret;
    }

    return HAL_SUCCESS;
}

hal_retcode skifio_deinit() {
    hal_gpio_group_set_intr(&GS.ctrl_pins.group, NULL, NULL);
    switch_ao_keys(false);

    hal_retcode st = hal_spi_disable(SPI_DEV_ID);
    if (st != HAL_SUCCESS) {
        return st;
    }
    hal_spi_deinit();
    return HAL_SUCCESS;
}

hal_retcode skifio_ao_enable() {
    switch_ao_keys(true);
    return HAL_SUCCESS;
}

hal_retcode skifio_ao_disable() {
    switch_ao_keys(false);
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

    // Store AO value
    memcpy(tx + 2, &out->ao, 4);

    // Store CRC
    calc_crc = calculate_crc16(tx, 6);
    memcpy(tx + 6, &calc_crc, 2);

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

    // Load AI values, temp and status
    const size_t in_data_len = SKIFIO_AI_CHANNEL_COUNT * 4 + 1 + 1;
    memcpy(in, rx, in_data_len);

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

hal_retcode skifio_do_write(SkifioDo value) {
    if ((value & ~((1 << SKIFIO_DO_SIZE) - 1)) != 0) {
        return HAL_INVALID_INPUT;
    }
    for (size_t i = 0; i < SKIFIO_DO_SIZE; ++i) {
        hal_gpio_pin_write(&GS.dio_pins.do_[i], (value & (1 << i)) != 0);
    }
    return HAL_SUCCESS;
}

SkifioDi skifio_di_read() {
    SkifioDi value = 0;
    for (size_t i = 0; i < SKIFIO_DI_SIZE; ++i) {
        if (hal_gpio_pin_read(&GS.dio_pins.di[i])) {
            value |= (SkifioDi)(1 << i);
        }
    }
    return value;
}

hal_retcode skifio_di_subscribe(SkifioDiCallback callback, void *data) {
    if (GS.di_callback != NULL) {
        return HAL_FAILURE;
    }
    GS.di_callback = callback;
    GS.di_user_data = data;
    return HAL_SUCCESS;
}

hal_retcode skifio_di_unsubscribe() {
    GS.di_callback = NULL;
    GS.di_user_data = NULL;
    return HAL_SUCCESS;
}
