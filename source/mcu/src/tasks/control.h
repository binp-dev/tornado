#pragma once

#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <common/config.h>
#include <utils/ringbuf.h>
#include <drivers/skifio.h>
#include <tasks/stats.h>

#include "config.h"

#define DAC_BUFFER_SIZE 1024
#define ADC_BUFFER_SIZE 256


typedef struct {
    RingBuffer queue;
    point_t last_point;
} DacBuffer;

typedef struct {
    RingBuffer queue;
} AdcBuffer;

typedef struct {
    SkifioDin in;
    SkifioDout out;
} DiscreteIo;

typedef struct {
    /// Semaphore to notify that something is ready.
    SemaphoreHandle_t *ready_sem;
    /// Discrete input has changed.
    volatile bool din_changed;
    /// Discrete output has changed.
    volatile bool dout_changed;
    /// Remaining DAC points to handle until notification.
    volatile size_t dac_remaining;
    /// Remaining ADC points to handle until notification.
    volatile size_t adc_remaining;
} ControlSync;

typedef struct {
    bool enabled;
    DiscreteIo dio;
    DacBuffer dac;
    AdcBuffer adcs[ADC_COUNT];
    ControlSync *sync;
    Statistics *stats;
} Control;

void control_sync_init(ControlSync *self, SemaphoreHandle_t *ready_sem);

void control_init(Control *self, ControlSync *sync, Statistics *stats);
void control_deinit(Control *self);

void control_enable(Control *self);
void control_disable(Control *self);

/// Start control tasks.
void control_run(Control *self);
