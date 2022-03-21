#pragma once

#include <stdlib.h>
#include <stdint.h>

#include <common/config.h>

#include "config.h"

#define STATS_REPORT_PERIOD_MS 10000

typedef volatile struct {
    int64_t sum;
    size_t count;
    point_t last;
    point_t min;
    point_t max;
} ValueStats;

typedef struct {
    size_t lost_empty;
    size_t lost_full;
    size_t req_exceed;
} DacStats;

typedef struct {
    ValueStats value;
    size_t lost_full;
} AdcStats;

typedef volatile struct {
#ifdef GENERATE_SYNC
    size_t clock_count;
#endif
    size_t sample_count;
    size_t max_intrs_per_sample;

    size_t crc_error_count;
    DacStats dac;
    AdcStats adcs[ADC_COUNT];
} Statistics;

void value_stats_reset(ValueStats *self);
void value_stats_update(ValueStats *self, point_t value);
void value_stats_print(ValueStats *self, const char *prefix);

void stats_reset(Statistics *self);
void stats_print(Statistics *self);

void stats_report_run(Statistics *self);
