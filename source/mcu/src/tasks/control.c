#include "control.h"

#include <hal/assert.h>
#include <hal/math.h>

#define RB_STRUCT DacRingBuffer
#define RB_PREFIX dac_rb
#define RB_ITEM point_t
#define RB_CAPACITY DAC_BUFFER_SIZE
#include <utils/ringbuf.inl>
#undef RB_STRUCT
#undef RB_PREFIX
#undef RB_ITEM
#undef RB_CAPACITY

#define RB_STRUCT AdcRingBuffer
#define RB_PREFIX adc_rb
#define RB_ITEM AdcArray
#define RB_CAPACITY ADC_BUFFER_SIZE
#include <utils/ringbuf.inl>
#undef RB_STRUCT
#undef RB_PREFIX
#undef RB_ITEM
#undef RB_CAPACITY

void control_init(Control *self, Statistics *stats) {
    self->dio.in = 0;
    self->dio.out = 0;

    self->dac.running = false;
    hal_assert_retcode(dac_rb_init(&self->dac.buffer));
    self->dac.last_point = 0x7fff;
    self->dac.counter = 0;

    hal_assert_retcode(adc_rb_init(&self->adc.buffer));
    self->adc.counter = 0;

    self->sync = NULL;

    hal_assert(stats != NULL);
    self->stats = stats;
}

void control_deinit(Control *self) {
    hal_assert_retcode(dac_rb_deinit(&self->dac.buffer));
    hal_assert_retcode(adc_rb_deinit(&self->adc.buffer));
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
            if (dac_rb_read(&self->dac.buffer, &dac_value, 1) == 1) {
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
            AdcArray adcs = {{0}};
            for (size_t i = 0; i < ADC_COUNT; ++i) {
                point_t value = input.adcs[i];
                adcs.points[i] = value;

                // Update ADC value statistics
                value_stats_update(&self->stats->adc.values[i], value);
            }
            // Push ADC point to buffer.
            if (adc_rb_write(&self->adc.buffer, &adcs, 1) != 1) {
                self->stats->adc.lost_full += 1;
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
