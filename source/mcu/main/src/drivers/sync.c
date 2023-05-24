#include "sync.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <fsl_common.h>
#include <fsl_iomuxc.h>

#include <hal/assert.h>
#include <hal/gpt.h>
#include <hal/gpio.h>
#include <hal/io.h>

// #define SYN_10K_MUX IOMUXC_SAI3_TXC_GPIO5_IO00
#define SYN_10K_MUX IOMUXC_SAI3_TXC_GPT1_COMPARE2
#define SYN_10K_PIN 5, 0

#define SYN_1_MUX IOMUXC_SAI3_RXD_GPIO4_IO30
// #define SYN_1_MUX IOMUXC_SAI3_RXD_GPT1_COMPARE1
#define SYN_1_PIN 4, 30

typedef struct {
    HalGpt gpt;
    HalGpioGroup group;
    HalGpioPin pins[2];
    volatile uint32_t counter;
} Sync;

static Sync SYNC;

static void update_pins() {
    // hal_gpio_pin_write(&SYNC.pins[0], SYNC.counter % 2 != 0);
}

void sync_init() {
    IOMUXC_SetPinMux(SYN_10K_MUX, 0u);
    IOMUXC_SetPinMux(SYN_1_MUX, 0u);

    hal_gpio_group_init(&SYNC.group);
    // hal_gpio_pin_init(&SYNC.pins[0], &SYNC.group, SYN_10K_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&SYNC.pins[1], &SYNC.group, SYN_1_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_DISABLED);

    SYNC.counter = 0;
    update_pins();

    hal_assert_retcode(hal_gpt_init(&SYNC.gpt, 1));
    hal_print("GPT initialized");
}

void sync_deinit() {
    hal_panic();
}

extern void user_sync_intr();

static void handle_sync(void *data) {
    // Update state and pins
    SYNC.counter += 1;
    update_pins();

    if (SYNC.counter % 2 == 1) {
        user_sync_intr();
    }
}

void sync_start(uint32_t period_us) {
    hal_assert_retcode(hal_gpt_start(&SYNC.gpt, 2, period_us / 2, handle_sync, NULL));
}

void sync_stop() {
    hal_gpt_stop(&SYNC.gpt);
}
