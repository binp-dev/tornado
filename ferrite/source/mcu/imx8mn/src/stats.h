#pragma once

#include <stdlib.h>
#include <stdint.h>

#include <skifio.h>


typedef struct {
    int64_t sum;
    int32_t last;
    int32_t min;
    int32_t max;
} AdcStats;

typedef struct {
    size_t buff_was_empty;
    size_t buff_was_full;
} DacWfStats;

typedef struct {
#ifdef GENERATE_SYNC
    uint32_t clock_count;
#endif
    uint32_t sample_count;
    uint32_t max_intrs_per_sample;
    AdcStats adcs[SKIFIO_ADC_CHANNEL_COUNT];
    DacWfStats dac_wf;
    size_t adc_buff_was_full[SKIFIO_ADC_CHANNEL_COUNT];
} Statistics;


extern volatile Statistics STATS;

void stats_reset();

void stats_print();
