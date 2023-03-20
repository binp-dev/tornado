#pragma once

#include <stdlib.h>
#include <stdint.h>

#include <common/config.h>

#include "config.h"

#define STATS_REPORT_PERIOD_MS 10000

typedef volatile struct {
    int64_t sum;
    uint32_t count;
    point_t last;
    point_t min;
    point_t max;
} ValueStats;

typedef struct {
    uint32_t lost_empty;
    uint32_t lost_full;
    uint32_t req_exceed;
} DacStats;

typedef struct {
    ValueStats values[ADC_COUNT];
    uint32_t lost_full;
} AdcStats;

typedef volatile struct {
    uint64_t clock_count;
    uint64_t sample_count;
    uint32_t max_intrs_per_sample;

    uint32_t crc_error_count;
    DacStats dac;
    AdcStats adc;
} Statistics;

void value_stats_reset(ValueStats *self);
void value_stats_update(ValueStats *self, point_t value);
void value_stats_print(ValueStats *self, const char *prefix);

void stats_reset(Statistics *self);
void stats_print(Statistics *self);

void stats_report_run(Statistics *self);
