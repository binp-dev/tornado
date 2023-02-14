use crate::{
    println,
    skifio::{AtomicDin, AtomicDout, Din, DinHandler, Skifio},
};
use alloc::sync::Arc;
use common::config::{Point, ADC_COUNT};
use core::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use freertos::{InterruptContext, Semaphore};
use ringbuf::{StaticConsumer, StaticProducer};

pub type DacPoint = Point;
pub type AdcPoint = [Point; ADC_COUNT];

pub const DAC_BUFFER_LEN: usize = 1024;
pub const ADC_BUFFER_LEN: usize = 384;

pub type DacProducer = StaticProducer<'static, DacPoint, DAC_BUFFER_LEN>;
pub type DacConsumer = StaticConsumer<'static, DacPoint, DAC_BUFFER_LEN>;

pub type AdcProducer = StaticProducer<'static, AdcPoint, ADC_BUFFER_LEN>;
pub type AdcConsumer = StaticConsumer<'static, AdcPoint, ADC_BUFFER_LEN>;

pub type Statistics = ();

/// Number of DAC points to write until notified.
/// FIXME: Adjust value.
const DAC_NOTIFY_EVERY: usize = 100;
/// Number of ADC points to read until notified.
/// FIXME: Adjust value.
const ADC_NOTIFY_EVERY: usize = 100;

pub struct ControlHandle {
    /// Semaphore to notify that something is ready.
    ready_sem: Semaphore,

    dac_enabled: AtomicBool,

    din: AtomicDin,
    dout: AtomicDout,

    /// Discrete input has changed.
    din_changed: AtomicBool,
    /// Discrete output has changed.
    dout_changed: AtomicBool,
}

struct ControlDac {
    running: bool,
    buffer: DacProducer,
    last_point: DacPoint,
    counter: usize,
}

struct ControlAdc {
    buffer: AdcConsumer,
    counter: usize,
}

pub struct Control {
    dac: ControlDac,
    adc: ControlAdc,
    handle: Arc<ControlHandle>,
}

impl ControlHandle {
    fn new() -> Self {
        Self {
            ready_sem: Semaphore::new_binary().unwrap(),
            dac_enabled: AtomicBool::new(false),
            din: AtomicDin::new(0),
            dout: AtomicDout::new(0),
            din_changed: AtomicBool::new(false),
            dout_changed: AtomicBool::new(false),
        }
    }
}

impl Control {
    pub fn new(dac_buf: DacProducer, adc_buf: AdcConsumer) -> (Self, Arc<ControlHandle>) {
        let handle = Arc::new(ControlHandle::new());
        (
            Self {
                dac: ControlDac {
                    running: false,
                    buffer: dac_buf,
                    last_point: 0x7fff,
                    counter: 0,
                },
                adc: ControlAdc {
                    buffer: adc_buf,
                    counter: 0,
                },
                handle: handle.clone(),
            },
            handle,
        )
    }

    fn make_din_handler(&self) -> impl DinHandler {
        let handle = self.handle.clone();
        move |context, din| {
            if handle.din.swap(din, Ordering::AcqRel) != din {
                handle.din_changed.fetch_or(true, Ordering::AcqRel);
                handle.ready_sem.give_from_isr(context);
            }
        }
    }
}
/*
fn control_task(&mut self) {
    self.skifio.
    hal_assert_retcode(skifio_din_subscribe(intr_din_handler, NULL));

    hal_log_info("Enter SkifIO loop");
    uint64_t prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;
    for (size_t k = 0;; ++k) {
        self.skifio.set_dac_state(enabled).unwrap();

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
*/
