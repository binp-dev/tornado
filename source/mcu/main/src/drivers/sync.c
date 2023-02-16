#include "sync.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <fsl_iomuxc.h>

#include <hal/assert.h>
#include <hal/gpt.h>


#define SYN_10K_MUX IOMUXC_SAI3_TXC_GPIO5_IO00
// #define SYN_10K_MUX IOMUXC_SAI3_TXC_GPT1_COMPARE2
#define SYN_10K_PIN 5, 0

#define SYN_1_MUX IOMUXC_SAI3_RXD_GPIO4_IO30
// #define SYN_1_MUX IOMUXC_SAI3_RXD_GPT1_COMPARE1
#define SYN_1_PIN 4, 30

#define GPT_CHANNEL 1

static void update_pins(SyncGenerator *self) {
    hal_gpio_pin_write(&self->pins[0], self->counter % 2 != 0);
}

void sync_generator_init(SyncGenerator *self) {
    IOMUXC_SetPinMux(SYN_10K_MUX, 0u);
    IOMUXC_SetPinMux(SYN_1_MUX, 0u);

    hal_gpio_group_init(&self->group);
    hal_gpio_pin_init(&self->pins[0], &self->group, SYN_10K_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&self->pins[1], &self->group, SYN_1_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_DISABLED);

    self->counter = 0;
    update_pins(self);

    hal_assert(hal_gpt_init(&self->gpt, 1) == HAL_SUCCESS);
    hal_log_info("GPT initialized");
}

void sync_generator_deinit(SyncGenerator *self) {
    hal_assert(hal_gpt_deinit(&self->gpt) == HAL_SUCCESS);
}

extern void user_sync_intr();

static void handle_gpt(void *data) {
    SyncGenerator *self = (SyncGenerator *)data;

    // Update state and pins
    self->counter += 1;
    update_pins(self);

    if (self->counter % 2 == 1) {
        user_sync_intr();
    }
}

void sync_generator_start(SyncGenerator *self, uint32_t period_us) {
    hal_assert(hal_gpt_start(&self->gpt, GPT_CHANNEL, period_us / 2, handle_gpt, (void *)self) == HAL_SUCCESS);
}

void sync_generator_stop(SyncGenerator *self) {
    hal_assert(hal_gpt_stop(&self->gpt) == HAL_SUCCESS);
}
