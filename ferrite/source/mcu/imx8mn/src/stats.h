#pragma once

#include <stdlib.h>
#include <stdint.h>

#include <skifio.h>


typedef struct {
#ifdef GENERATE_SYNC
    uint32_t clock_count;
#endif
    uint32_t sample_count;
    uint32_t max_intrs_per_sample;
    int32_t min_adc;
    int32_t max_adc;
    int32_t last_adcs[SKIFIO_ADC_CHANNEL_COUNT];
} Statistics;


extern volatile Statistics STATS;

void stats_reset();

void stats_print();
