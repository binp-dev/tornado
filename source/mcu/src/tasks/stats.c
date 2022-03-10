#include "stats.h"

#include <FreeRTOS.h>
#include <task.h>

#include <hal/log.h>

void _stats_value_reset(ValueStats *value) {
    value->count = 0;
    value->sum = 0;
    value->max = 0;
    value->min = 0;
    value->last = 0;
}

void stats_reset(Statistics *stats) {
#ifdef GENERATE_SYNC
    stats->clock_count = 0;
#endif
    stats->sample_count = 0;
    stats->max_intrs_per_sample = 0;

    stats->crc_error_count = 0;

    stats->dac.lost_empty = 0;
    stats->dac.lost_full = 0;

    for (size_t i = 0; i < ADC_COUNT; ++i) {
        stats->adcs[i].lost_full = 0;
        _stats_value_reset(&stats->adcs[i].value);
    }
}

void stats_print(Statistics *stats) {
#ifdef GENERATE_SYNC
    hal_log_info("clock_count: %ld", stats->clock_count);
#endif
    hal_log_info("sample_count: %ld", stats->sample_count);
    hal_log_info("max_intrs_per_sample: %ld", stats->max_intrs_per_sample);

    for (size_t j = 0; j < ADC_COUNT; ++j) {
        volatile AdcStats *adc = &stats->adcs[j];
        hal_log_info("adc[%d]:", j);
        hal_log_info("    last: (0x%08lx) %ld", adc->last, adc->last);
        hal_log_info("    min: (0x%08lx) %ld", adc->min, adc->min);
        hal_log_info("    max: (0x%08lx) %ld", adc->max, adc->max);
        int32_t avg = (int32_t)(adc->sum / stats->sample_count);
        hal_log_info("    avg: (0x%08lx) %ld", avg, avg);
    }

    hal_log_info("dac waveform:");
    hal_log_info("    buffer was full: %ld", stats->dac_wf.buff_was_full);
    hal_log_info("    buffer was empty: %ld", stats->dac_wf.buff_was_empty);

    for (size_t j = 0; j < ADC_COUNT; ++j) {
        hal_log_info("adc waveform[%d]:", j);
        hal_log_info("    buffer was full: %ld", stats->adc_buff_was_full[j]);
    }
}

static void stats_task(void *param) {
    Statistics *stats = (Statistics *)param;

    for (size_t i = 0;; ++i) {
        hal_log_info("");
        stats_print(stats);
        hal_log_info("din: 0x%02lx", (uint32_t)DIO.in);
        hal_log_info("dout: 0x%01lx", (uint32_t)DIO.out);
        vTaskDelay(STATS_REPORT_PERIOD_MS);
    }
}

void stats_report_run(Statistics *stats) {
    hal_log_info("Starting statistics report task");
    hal_assert(
        xTaskCreate(stats_task, "Statistics report task", TASK_STACK_SIZE, (void *)stats, STATISTICS_TASK_PRIORITY, NULL)
        == pdPASS);
}
