#pragma once

#include <stdint.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <hal/gpio.h>

#include <tasks/stats.h>

typedef struct {
    uint32_t period_us;
    HalGpioGroup group;
    HalGpioPin pins[2];
    SemaphoreHandle_t sem;
    volatile uint32_t counter;

    Statistics *stats;
} SyncGenerator;

void sync_generator_init(SyncGenerator *self, uint32_t period_us, Statistics *stats);

void sync_generator_run(SyncGenerator *self);
