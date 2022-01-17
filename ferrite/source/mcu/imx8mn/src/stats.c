#include "stats.h"

#include <hal/log.h>

#include "skifio.h"


volatile Statistics STATS = {
#ifdef GENERATE_SYNC
    0,
#endif
    0,
    0,
    0,
    0,
    {0},
};


void stats_reset() {
#ifdef GENERATE_SYNC
    STATS.clock_count = 0;
#endif
    STATS.sample_count = 0;
    STATS.max_intrs_per_sample = 0;
    STATS.min_adc = 0;
    STATS.max_adc = 0;
}

void stats_print() {
#ifdef GENERATE_SYNC
    hal_log_info("clock_count: %ld", STATS.clock_count);
#endif
    hal_log_info("sample_count: %ld", STATS.sample_count);
    hal_log_info("max_intrs_per_sample: %ld", STATS.max_intrs_per_sample);
    int32_t v_min = STATS.min_adc;
    hal_log_info("min_adc: (0x%08lx) %ld", v_min, v_min);
    int32_t v_max = STATS.max_adc;
    hal_log_info("max_adc: (0x%08lx) %ld", v_max, v_max);

    for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
        int32_t v = STATS.last_adcs[j];
        hal_log_info("adc%d: (0x%08lx) %ld", j, v, v);
    }
}
