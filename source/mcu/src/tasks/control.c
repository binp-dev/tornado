#include "control.h"

#include <hal/assert.h>

void control_init(Control *control, Statistics *stats) {
    control->dio.in = 0;
    control->dio.out = 0;

    hal_assert_retcode(rb_init(&control->dac.queue, DAC_BUFFER_SIZE));
    control->dac.last_point = 0;

    for (size_t i = 0; i < ADC_COUNT; ++i) {
        hal_assert_retcode(rb_init(&control->adcs[i].queue, ADC_BUFFER_SIZE));
    }

    control->ready_sem = NULL;

    control->stats = stats;
}

void control_deinit(Control *control) {
    hal_assert_retcode(rb_deinit(&control->dac.queue));

    for (size_t i = 0; i < ADC_COUNT; ++i) {
        hal_assert_retcode(rb_deinit(&control->adcs[i].queue));
    }
}

static void din_handler(void *data, SkifioDin value) {
    Control *control = (Control *)control;
    control->dio.in = value;
    BaseType_t hptw = pdFALSE;
    xSemaphoreGiveFromISR(*control->ready_sem, &hptw);
    portYIELD_FROM_ISR(hptw);
}

static void control_task(void *param) {
    Control *control = (Control *)param;

    hal_log_info("SkifIO driver init");
    hal_assert(skifio_init() == HAL_SUCCESS);
    // hal_assert(skifio_din_subscribe(din_handler, NULL) == HAL_SUCCESS);

    SkifioInput input = {{0}};
    SkifioOutput output = {0};

    hal_log_info("Enter SkifIO loop");
    uint64_t prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;
    for (size_t i = 0;; ++i) {
        hal_retcode ret;
        bool need_send_din = false;
        bool need_send_adc = false;
        bool need_dac_wf_req = false;


        ret = skifio_wait_ready(1000);
        if (ret == HAL_TIMED_OUT) {
            hal_log_info("SkifIO timeout %d", i);
            continue;
        }
        hal_assert(ret == HAL_SUCCESS);

        SkifioDin din = skifio_din_read();
        if (din != control->dio.in) {
            need_send_din = true;
        }

        STATS.max_intrs_per_sample = hal_max(
            STATS.max_intrs_per_sample,
            (uint32_t)(_SKIFIO_DEBUG_INFO.intr_count - prev_intr_count) //
        );
        prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;

        int32_t dac_wf_value = 0; // TODO: set zero in Volts, not in code
        if (DAC.was_set) {
            size_t read_data_size = xStreamBufferReceive(DAC.queue, &dac_wf_value, sizeof(int32_t), 0);
            hal_assert(read_data_size % sizeof(int32_t) == 0); // TODO: Delete after debug?
            if (read_data_size == 0) {
                ++STATS.dac_wf.buff_was_empty;
            }
        }

        size_t free_space_in_buff = xStreamBufferSpacesAvailable(DAC.queue) / sizeof(int32_t);
        if (free_space_in_buff >= FREE_SPACE_IN_BUFF_FOR_DAC_WF_REQUEST) {
            need_dac_wf_req = true;
        }

        output.dac = (int16_t)dac_wf_value;

        ret = skifio_transfer(&output, &input);
        hal_assert(ret == HAL_SUCCESS || ret == HAL_INVALID_DATA); // Ignore CRC check error


        for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
            volatile AdcStats *stats = &STATS.adcs[j];
            int32_t value = input.adcs[j];

            if (STATS.sample_count == 0) {
                stats->min = value;
                stats->max = value;
            } else {
                stats->min = hal_min(stats->min, value);
                stats->max = hal_max(stats->max, value);
            }
            stats->last = value;
            stats->sum += value;

            if (IOC_STARTED) {
                size_t added_data_size = xStreamBufferSend(ADCS[j].queue, &value, sizeof(int32_t), 0);
                hal_assert(added_data_size % sizeof(int32_t) == 0); // TODO: Delete after debug?
                if (added_data_size == 0) {
                    ++STATS.adc_buff_was_full[j];
                }

                size_t elems_in_buff = xStreamBufferBytesAvailable(ADCS[j].queue) / sizeof(int32_t);
                if (elems_in_buff >= ADC_MSG_MAX_POINTS) {
                    need_send_adc = true;
                }
            }
        }

        if (need_send_din || need_send_adc || need_dac_wf_req) {
            hal_assert(control->ready_sem != NULL);
            xSemaphoreGive(RPMSG_SEND_SEM);
        }

        STATS.sample_count += 1;
    }

    hal_log_error("End of task_skifio()");
    hal_panic();

    hal_assert(skifio_deinit() == HAL_SUCCESS);
}

void control_set_ready_sem(Control *control, SemaphoreHandle_t *ready_sem) {
    control->ready_sem = ready_sem;
}

void control_run(Control *control) {
    hal_log_info("Starting control task");
    hal_assert(xTaskCreate(control_task, "Control task", TASK_STACK_SIZE, NULL, CONTROL_TASK_PRIORITY, NULL) == pdPASS);
}
