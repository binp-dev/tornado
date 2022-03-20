#include "stats.h"

#include <FreeRTOS.h>
#include <task.h>

#include <hal/assert.h>
#include <hal/log.h>
#include <hal/math.h>

void value_stats_reset(ValueStats *self) {
    self->count = 0;
    self->sum = 0;
    self->max = 0;
    self->min = 0;
    self->last = 0;
}

void value_stats_update(ValueStats *self, point_t value) {
    if (self->count == 0) {
        self->min = value;
        self->max = value;
    } else {
        self->min = hal_min(self->min, value);
        self->max = hal_max(self->max, value);
    }
    self->last = value;
    self->sum += value;
}

void value_stats_print(ValueStats *self, const char *prefix) {
    hal_log_info("%slast: (0x%08lx) %ld", prefix, self->last, self->last);
    hal_log_info("%smin: (0x%08lx) %ld", prefix, self->min, self->min);
    hal_log_info("%smax: (0x%08lx) %ld", prefix, self->max, self->max);
    int32_t avg = (int32_t)(self->sum / self->count);
    hal_log_info("%savg: (0x%08lx) %ld", prefix, avg, avg);
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
        value_stats_reset(&stats->adcs[i].value);
    }
}

void stats_print(Statistics *stats) {
#ifdef GENERATE_SYNC
    hal_log_info("clock_count: %d", stats->clock_count);
#endif
    hal_log_info("sample_count: %d", stats->sample_count);
    hal_log_info("max_intrs_per_sample: %d", stats->max_intrs_per_sample);

    hal_log_info("DAC:");
    hal_log_info("    Points lost because buffer was full: %d", stats->dac.lost_full);
    hal_log_info("    Points lost because buffer was empty: %d", stats->dac.lost_empty);

    for (size_t j = 0; j < ADC_COUNT; ++j) {
        hal_log_info("ADC[%d]:", j);
        hal_log_info("    Points lost because buffer was full: %d", stats->adcs[j].lost_full);
        hal_log_info("    Mertics:");
        value_stats_print(&stats->adcs[j].value, "        ");
    }
}

static void stats_task(void *param) {
    Statistics *stats = (Statistics *)param;

    for (size_t i = 0;; ++i) {
        hal_log_info("");
        stats_print(stats);
        vTaskDelay(STATS_REPORT_PERIOD_MS);
    }
}

void stats_report_run(Statistics *stats) {
    hal_log_info("Starting statistics report task");
    hal_assert(
        xTaskCreate(stats_task, "Statistics report task", TASK_STACK_SIZE, (void *)stats, STATISTICS_TASK_PRIORITY, NULL)
        == pdPASS);
}
