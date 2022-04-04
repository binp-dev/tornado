#include "control.h"

#include <hal/assert.h>
#include <hal/math.h>

void control_init(Control *self, Statistics *stats) {
    self->dio.in = 0;
    self->dio.out = 0;

    self->dac.running = false;
    hal_assert_retcode(rb_init(&self->dac.buffer, DAC_BUFFER_SIZE));
    self->dac.last_point = 0;
    self->dac.counter = 0;

    for (size_t i = 0; i < ADC_COUNT; ++i) {
        hal_assert_retcode(rb_init(&self->adc.buffers[i], ADC_BUFFER_SIZE));
    }
    self->adc.counter = 0;

    self->sync = NULL;

    hal_assert(stats != NULL);
    self->stats = stats;
}

void control_deinit(Control *self) {
    hal_assert_retcode(rb_deinit(&self->dac.buffer));

    for (size_t i = 0; i < ADC_COUNT; ++i) {
        hal_assert_retcode(rb_deinit(&self->adc.buffers[i]));
    }
}

void control_set_sync(Control *self, ControlSync *sync) {
    hal_assert(sync != NULL);
    hal_assert(sync->ready_sem != NULL);
    self->sync = sync;
}

void control_dac_start(Control *self) {
    skifio_dac_enable();
    self->dac.running = true;
}

void control_dac_stop(Control *self) {
    self->dac.running = false;
    skifio_dac_disable();
}

void control_sync_init(ControlSync *self, SemaphoreHandle_t *ready_sem, size_t dac_chunk_size, size_t adc_chunk_size) {
    self->ready_sem = ready_sem;

    self->dac_notify_every = dac_chunk_size;
    self->adc_notify_every = adc_chunk_size;

    self->din_changed = false;
    self->dout_changed = false;
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

static void control_task(void *param) {
    Control *self = (Control *)param;

    // Check that sync is set.
    hal_assert(self->sync != NULL);

    hal_log_info("SkifIO driver init");
    hal_assert_retcode(skifio_init());
    hal_assert_retcode(skifio_din_subscribe(intr_din_handler, NULL));

    hal_log_info("Enter SkifIO loop");
    uint64_t prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;
    for (size_t k = 0;; ++k) {
        bool ready = false;

        // Wait for 10 kHz sync signal
        {
            hal_retcode ret = skifio_wait_ready(1000);
            if (ret == HAL_TIMED_OUT) {
                hal_log_warn("SkifIO timeout %d", k);
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
        if (self->dac.running) {
            if (rb_read(&self->dac.buffer, &dac_value, 1) == 1) {
                self->dac.last_point = dac_value;
                // Decrement DAC notification counter.
                if (self->dac.counter > 0) {
                    self->dac.counter -= 1;
                } else {
                    self->dac.counter = self->sync->dac_notify_every - 1;
                    ready = true;
                }
            } else {
                self->stats->dac.lost_empty += 1;
            }
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

            // Handle ADCs
            for (size_t i = 0; i < ADC_COUNT; ++i) {
                point_t value = input.adcs[i];
                volatile AdcStats *adc_stats = &self->stats->adcs[i];

                // Update ADC value statistics
                value_stats_update(&adc_stats->value, value);

                // Push ADC point to buffer.
                if (rb_write(&self->adc.buffers[i], &value, 1) != 1) {
                    self->stats->adcs[i].lost_full += 1;
                }
            }
            // Decrement ADC notification counter.
            if (self->adc.counter > 0) {
                self->adc.counter -= 1;
            } else {
                self->adc.counter = self->sync->adc_notify_every - 1;
                ready = true;
            }
        }

        if (ready) {
            // Notify
            xSemaphoreGive(*self->sync->ready_sem);
        }

        self->stats->sample_count += 1;
    }

    // This task must never end.
    hal_unreachable();

    hal_assert_retcode(skifio_deinit());
}

void control_run(Control *self) {
    hal_assert(xTaskCreate(control_task, "control", TASK_STACK_SIZE, (void *)self, CONTROL_TASK_PRIORITY, NULL) == pdPASS);
}