#include "sync.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <fsl_iomuxc.h>

#include <hal/assert.h>
#include <hal/gpt.h>

#include "stats.h"


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

void sync_generator_init(SyncGenerator *self, uint32_t period_us, Statistics *stats) {
    self->period_us = period_us;

    IOMUXC_SetPinMux(SYN_10K_MUX, 0u);
    IOMUXC_SetPinMux(SYN_1_MUX, 0u);

    hal_gpio_group_init(&self->group);
    hal_gpio_pin_init(&self->pins[0], &self->group, SYN_10K_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&self->pins[1], &self->group, SYN_1_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_DISABLED);

    self->sem = xSemaphoreCreateBinary();
    hal_assert(self->sem != NULL);

    self->stats = stats;

    self->counter = 0;
    update_pins(self);
}

static void handle_gpt(void *data) {
    BaseType_t hptw = pdFALSE;
    SyncGenerator *self = (SyncGenerator *)data;

    // Update state and pins
    self->counter += 1;
    update_pins(self);

    // Notify target task
    xSemaphoreGiveFromISR(self->sem, &hptw);

    // Yield to higher priority task
    portYIELD_FROM_ISR(hptw);
}

void sync_generator_task(void *param) {
    SyncGenerator *self = (SyncGenerator *)param;

    HalGpt gpt;
    hal_assert(hal_gpt_init(&gpt, 1) == HAL_SUCCESS);
    hal_log_info("GPT initialized");

    hal_assert(hal_gpt_start(&gpt, GPT_CHANNEL, self->period_us / 2, handle_gpt, (void *)self) == HAL_SUCCESS);
    for (size_t i = 0;; ++i) {
        if (xSemaphoreTake(self->sem, 10000) != pdTRUE) {
            hal_log_info("GPT semaphore timeout %x", i);
            continue;
        }

        if (self->counter % 2 == 1) {
            self->stats->clock_count += 1;
        }
    }
    hal_panic();

    hal_assert(hal_gpt_stop(&gpt) == HAL_SUCCESS);
    hal_assert(hal_gpt_deinit(&gpt) == HAL_SUCCESS);
}

void sync_generator_run(SyncGenerator *self) {
    hal_assert(xTaskCreate(sync_generator_task, "sync", TASK_STACK_SIZE, (void *)self, SYNC_TASK_PRIORITY, NULL) == pdPASS);
}
