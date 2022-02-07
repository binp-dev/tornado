#include "stats.h"

#include <hal/log.h>

#include "skifio.h"


volatile Statistics STATS = {
#ifdef GENERATE_SYNC
    0,
#endif
    0,
    0,
    {{0, 0, 0, 0}},
    {0, 0},
    {0}
};


void stats_reset() {
#ifdef GENERATE_SYNC
    STATS.clock_count = 0;
#endif
    STATS.sample_count = 0;
    STATS.max_intrs_per_sample = 0;
    for (size_t i = 0; i < SKIFIO_ADC_CHANNEL_COUNT; ++i) {
        STATS.adcs[i].sum = 0;
    }
}

void stats_print() {
#ifdef GENERATE_SYNC
    hal_log_info("clock_count: %ld", STATS.clock_count);
#endif
    hal_log_info("sample_count: %ld", STATS.sample_count);
    hal_log_info("max_intrs_per_sample: %ld", STATS.max_intrs_per_sample);

    for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
        volatile AdcStats *adc = &STATS.adcs[j];
        hal_log_info("adc[%d]:", j);
        hal_log_info("    last: (0x%08lx) %ld", adc->last, adc->last);
        hal_log_info("    min: (0x%08lx) %ld", adc->min, adc->min);
        hal_log_info("    max: (0x%08lx) %ld", adc->max, adc->max);
        int32_t avg = (int32_t)(adc->sum / STATS.sample_count);
        hal_log_info("    avg: (0x%08lx) %ld", avg, avg);
        
    }

    hal_log_info("dac waveform:");
    hal_log_info("    buffer was full: %ld", STATS.dac_wf.buff_was_full);
    hal_log_info("    buffer was empty: %ld", STATS.dac_wf.buff_was_empty);

    for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
        hal_log_info("adc waveform[%d]:", j);
        hal_log_info("    buffer was full: %ld", STATS.adc_buff_was_full[j]);
        
    }
}
