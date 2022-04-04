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
    bool running;
    RingBuffer buffer;
    point_t last_point;
    size_t counter;
} ControlDac;

typedef struct {
    RingBuffer buffers[ADC_COUNT];
    size_t counter;
} ControlAdc;

typedef struct {
    volatile SkifioDin in;
    volatile SkifioDout out;
} ControlDio;

typedef struct {
    /// Semaphore to notify that something is ready.
    SemaphoreHandle_t *ready_sem;

    /// Number of DAC points to write until notified.
    volatile size_t dac_notify_every;
    /// Number of ADC points to read until notified.
    volatile size_t adc_notify_every;

    /// Discrete input has changed.
    volatile bool din_changed;
    /// Discrete output has changed.
    volatile bool dout_changed;
} ControlSync;

typedef struct {
    ControlDio dio;
    ControlDac dac;
    ControlAdc adc;
    ControlSync *sync;
    Statistics *stats;
} Control;

void control_sync_init(ControlSync *self, SemaphoreHandle_t *ready_sem, size_t dac_chunk_size, size_t adc_chunk_size);

void control_init(Control *self, Statistics *stats);
void control_deinit(Control *self);

void control_set_sync(Control *self, ControlSync *sync);

void control_dac_start(Control *self);
void control_dac_stop(Control *self);

/// Start control tasks.
void control_run(Control *self);