#include "control.h"

#include <hal/assert.h>

void control_init(Control *self, ControlSync *sync, Statistics *stats) {
    self->enabled = false;

    self->dio.in = 0;
    self->dio.out = 0;

    hal_assert_retcode(rb_init(&self->dac.queue, DAC_BUFFER_SIZE));
    self->dac.last_point = 0;

    for (size_t i = 0; i < ADC_COUNT; ++i) {
        hal_assert_retcode(rb_init(&self->adcs[i].queue, ADC_BUFFER_SIZE));
    }

    hal_assert(sync != NULL);
    hal_assert(sync->ready_sem != NULL);
    self->sync = sync;

    hal_assert(stats != NULL);
    self->stats = stats;
}

void control_deinit(Control *self) {
    hal_assert_retcode(rb_deinit(&self->dac.queue));

    for (size_t i = 0; i < ADC_COUNT; ++i) {
        hal_assert_retcode(rb_deinit(&self->adcs[i].queue));
    }
}

void control_enable(Control *self) {
    skifio_dac_enable();
    self->enabled = true;
}

void control_disable(Control *self) {
    self->enabled = false;
    skifio_dac_disable();
}

void control_sync_init(ControlSync *self, SemaphoreHandle_t *ready_sem) {
    self->ready_sem = ready_sem;

    self->din_changed = false;
    self->dout_changed = false;

    self->dac_remaining = 0;
    self->adc_remaining = 0;
}

static bool update_din(Control *self) {
    SkifioDin din = skifio_din_read();
    if (din != self->dio.in) {
        self->dio.in = din;
        self->sync->din_changed = true;
        return true;
    } else {
        return false;
    }
}

static void intr_din_handler(void *data, SkifioDin value) {
    Control *self = (Control *)self;
    update_din(self);
    BaseType_t hptw = pdFALSE;
    if (self->sync->din_changed) {
        xSemaphoreGiveFromISR(*self->sync->ready_sem, &hptw);
    }
    portYIELD_FROM_ISR(hptw);
}

static bool decrement_remaining(volatile size_t *counter) {
    if (*counter > 0) {
        *counter -= 1;
        if (*counter == 0) {
            return true;
        }
    }
    return false;
}

static void control_task(void *param) {
    Control *self = (Control *)param;

    hal_log_info("SkifIO driver init");
    hal_assert_retcode(skifio_init());
    hal_assert_retcode(skifio_din_subscribe(intr_din_handler, NULL));

    hal_log_info("Enter SkifIO loop");
    uint64_t prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;
    for (size_t i = 0;; ++i) {
        bool ready = false;

        // Wait for 10 kHz sync signal
        {
            hal_retcode ret = skifio_wait_ready(1000);
            if (ret == HAL_TIMED_OUT) {
                hal_log_warn("SkifIO timeout %d", i);
                continue;
            }
            hal_assert_retcode(ret);
        }

        // Write discrete output
        if (self->sync->dout_changed) {
            hal_assert_retcode(skifio_dout_write(self->dio.out));
            self->sync->dout_changed = false;
        }

        // Read discrete input
        ready |= update_din(self);

        // Statistics: detect 10 kHz sync signal loss
        self->stats->max_intrs_per_sample = hal_max(
            self->stats->max_intrs_per_sample,
            (uint32_t)(_SKIFIO_DEBUG_INFO.intr_count - prev_intr_count) //
        );
        prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;

        // Fetch next DAC value from buffer
        int32_t dac_value = self->dac.last_point;
        if (rb_read(&self->dac.queue, &dac_value, 1) == 1) {
            self->dac.last_point = dac_value;
            ready |= decrement_remaining(&self->sync->dac_remaining);
        } else {
            self->stats->dac.lost_empty += 1;
        }

        // Transfer DAC/ADC values to/from SkifIO board.
        {
            SkifioInput input = {{0}};
            SkifioOutput output = {0};

            output.dac = (int16_t)dac_value;
            hal_retcode ret = skifio_transfer(&output, &input);
            if (ret == HAL_INVALID_DATA) {
                // CRC check error
                self->stats->crc_error_count += 1;
                ret = HAL_SUCCESS;
            }
            hal_assert_retcode(ret);

            for (size_t j = 0; j < ADC_COUNT; ++j) {
                point_t value = input.adcs[j];
                volatile AdcStats *adc_stats = &self->stats->adcs[j];

                // Update ADC value statistics
                value_stats_update(&adc_stats->value, value);

                // Push ADC point to buffer
                self->stats->adcs[j].lost_full += rb_overwrite(&self->adcs[j].queue, &value, 1);
            }
            ready |= decrement_remaining(&self->sync->adc_remaining);
        }

        if (ready) {
            xSemaphoreGive(*self->sync->ready_sem);
        }

        self->stats->sample_count += 1;
    }

    // This task must never end.
    hal_unreachable();

    hal_assert_retcode(skifio_deinit());
}

void control_run(Control *self) {
    hal_log_info("Starting control task");
    hal_assert(xTaskCreate(control_task, "Control task", TASK_STACK_SIZE, (void *)self, CONTROL_TASK_PRIORITY, NULL) == pdPASS);
}
