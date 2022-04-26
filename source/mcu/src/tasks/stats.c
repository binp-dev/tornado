#include "stats.h"

#include <FreeRTOS.h>
#include <task.h>

#include <hal/assert.h>
#include <hal/log.h>
#include <hal/math.h>

void value_stats_reset(ValueStats *self) {
    self->count = 0;
    self->sum = 0;
    self->max = -0x80000000;
    self->min = 0x7fffffff;
    self->last = 0;
}

void value_stats_update(ValueStats *self, point_t value) {
    self->min = hal_min(self->min, value);
    self->max = hal_max(self->max, value);
    self->last = value;
    self->sum += value;
    self->count += 1;
}

void value_stats_print(ValueStats *self, const char *prefix) {
    hal_log_info("%slast: (0x%08lx) %ld", prefix, self->last, self->last);
    hal_log_info("%smin: (0x%08lx) %ld", prefix, self->min, self->min);
    hal_log_info("%smax: (0x%08lx) %ld", prefix, self->max, self->max);
    if (self->count != 0) {
        int32_t avg = (int32_t)(self->sum / (int64_t)self->count);
        hal_log_info("%savg: (0x%08lx) %ld", prefix, avg, avg);
    }
}

void stats_reset(Statistics *self) {
    self->clock_count = 0;
    self->sample_count = 0;
    self->max_intrs_per_sample = 0;

    self->crc_error_count = 0;

    self->dac.lost_empty = 0;
    self->dac.lost_full = 0;

    self->adc.lost_full = 0;
    for (size_t i = 0; i < ADC_COUNT; ++i) {
        value_stats_reset(&self->adc.values[i]);
    }
}

void stats_print(Statistics *self) {
    hal_log_info("");

    // Number of 10 kHz sync signals captured.
    hal_log_info("clock_count: %ld", (uint32_t)self->clock_count);
    // Number of SkifIO `SMP_RDY` signals captured.
    hal_log_info("sample_count: %ld", (uint32_t)self->sample_count);
    // Maximum number of `SMP_RDY` per SkifIO communication session.
    // If it isn't equal to `1` that means that we lose some signals.
    hal_log_info("max_intrs_per_sample: %ld", self->max_intrs_per_sample);

    // Count of CRC16 mismatches in SkifIO communication.
    hal_log_info("crc_error_count: %ld", self->crc_error_count);

    hal_log_info("dac:");
    // Number of points lost because the DAC buffer was empty.
    hal_log_info("    lost_empty: %ld", self->dac.lost_empty);
    // Number of points lost because the DAC buffer was full.
    hal_log_info("    lost_full: %ld", self->dac.lost_full);
    // IOC sent more points than were requested.
    hal_log_info("    req_exceed: %ld", self->dac.req_exceed);

    hal_log_info("adc:");
    // Number of points lost because the ADC buffer was full.
    hal_log_info("    lost_full: %ld", self->adc.lost_full);
    for (size_t i = 0; i < ADC_COUNT; ++i) {
        hal_log_info("    channel %d:", i);
        value_stats_print(&self->adc.values[i], "        ");
    }
}

static void stats_task(void *param) {
    Statistics *stats = (Statistics *)param;

    for (;;) {
        stats_print(stats);
        vTaskDelay(STATS_REPORT_PERIOD_MS);
    }
}

void stats_report_run(Statistics *self) {
    hal_assert(xTaskCreate(stats_task, "stats", TASK_STACK_SIZE, (void *)self, STATISTICS_TASK_PRIORITY, NULL) == pdPASS);
}
