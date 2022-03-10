#pragma once

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
    DiscreteIo dio;
    DacBuffer dac;
    AdcBuffer adcs[ADC_COUNT];
    SemaphoreHandle_t *ready_sem;
    Statistics *stats;
} Control;


void control_init(Control *control, Statistics *stats);
void control_deinit(Control *control);

void control_set_ready_sem(Control *control, SemaphoreHandle_t *ready_sem);

/// Start control tasks.
void control_run(Control *control);
