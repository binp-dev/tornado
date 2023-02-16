#pragma once

#include <stdint.h>

#include <hal/gpio.h>
#include <hal/gpt.h>

typedef struct {
    HalGpioGroup group;
    HalGpioPin pins[2];
    volatile uint32_t counter;
    HalGpt gpt;
} SyncGenerator;

void sync_generator_init(SyncGenerator *self);
void sync_generator_deinit(SyncGenerator *self);

void sync_generator_start(SyncGenerator *self, uint32_t period_us);
void sync_generator_stop(SyncGenerator *self);
