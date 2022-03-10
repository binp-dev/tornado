#include "rpmsg.h"


void send_adc_wf_data() {
    for (int i = 0; i < SKIFIO_ADC_CHANNEL_COUNT; ++i) {
        size_t elems_in_buff = xStreamBufferBytesAvailable(ADCS[i].queue) / sizeof(int32_t);
        if (elems_in_buff >= ADC_MSG_MAX_POINTS) {
            uint8_t *buffer = NULL;
            size_t len = 0;
            hal_assert(hal_rpmsg_alloc_tx_buffer(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
            IppMcuMsg *adc_wf_msg = (IppMcuMsg *)buffer;
            adc_wf_msg->type = IPP_MCU_MSG_ADC_WF;
            adc_wf_msg->adc_wf.index = i;

            size_t sent_data_size = xStreamBufferReceive(
                ADCS[i].queue,
                &(adc_wf_msg->adc_wf.elements.data[0]),
                ADC_MSG_MAX_POINTS * sizeof(int32_t),
                0);
            adc_wf_msg->adc_wf.elements.len = sent_data_size / sizeof(int32_t);
            // hal_log_debug("Sent waveform of size: %d", sent_data_size / sizeof(int32_t));
            hal_assert(sent_data_size / sizeof(int32_t) == ADC_MSG_MAX_POINTS); // TODO: Delete after debug?

            hal_assert(hal_rpmsg_send_nocopy(&RPMSG_CHANNEL, buffer, ipp_mcu_msg_size(adc_wf_msg)) == HAL_SUCCESS);
        }
    }
}

void send_dac_wf_request() {
    size_t free_space_in_buff = xStreamBufferSpacesAvailable(DAC.queue) / sizeof(int32_t);

    if (free_space_in_buff >= FREE_SPACE_IN_BUFF_FOR_DAC_WF_REQUEST && !DAC.waiting_for_data) {
        DAC.waiting_for_data = true;
        uint8_t *buffer = NULL;
        size_t len = 0;
        hal_assert(hal_rpmsg_alloc_tx_buffer(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);

        IppMcuMsg *msg = (IppMcuMsg *)buffer;
        msg->type = IPP_MCU_MSG_DAC_WF_REQ;
        msg->dac_wf_req.count = 0; // TODO: Send requested points count

        hal_assert(hal_rpmsg_send_nocopy(&RPMSG_CHANNEL, buffer, ipp_mcu_msg_size(msg)) == HAL_SUCCESS);
    }
}

void send_din() {
    SkifioDin din = skifio_din_read();
    if (din == DIO.in) {
        return;
    }

    DIO.in = din;
    uint8_t *buffer = NULL;
    size_t len = 0;
    hal_assert(hal_rpmsg_alloc_tx_buffer(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
    IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
    mcu_msg->type = IPP_MCU_MSG_DIN_VAL;
    mcu_msg->din_val.value = DIO.in;
    hal_assert(hal_rpmsg_send_nocopy(&RPMSG_CHANNEL, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);
}

static void task_rpmsg_send(void *param) {
    for (;;) {
        if (!IOC_STARTED) {
            // FIXME:
            vTaskDelay(10);
            continue;
        }
        xSemaphoreTake(RPMSG_SEND_SEM, portMAX_DELAY);

        send_din();
        send_adc_wf_data();
        send_dac_wf_request();
    }
}

static void task_rpmsg_recv(void *param) {
    hal_rpmsg_init();

    hal_assert(hal_rpmsg_create_channel(&RPMSG_CHANNEL, 0) == HAL_SUCCESS);
#ifdef HAL_PRINT_RPMSG
    hal_io_rpmsg_init(&RPMSG_CHANNEL);
#endif
    hal_log_info("RPMSG channel created");

    // Receive message

    uint8_t *buffer = NULL;
    size_t len = 0;
    hal_rpmsg_recv_nocopy(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER);
    hal_assert(strncmp((const char *)buffer, "hello world!", len) == 0);
    hal_log_info("hello world!");
    hal_rpmsg_free_rx_buffer(&RPMSG_CHANNEL, buffer);
    buffer = NULL;
    len = 0;

    // Start messaging

    const IppAppMsg *app_msg = NULL;
    hal_rpmsg_recv_nocopy(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER);
    app_msg = (const IppAppMsg *)buffer;
    if (app_msg->type == IPP_APP_MSG_CONNECT) {
        hal_log_info("Start message received");
        IOC_STARTED = true;
        xSemaphoreGive(RPMSG_SEND_SEM);
    } else {
        hal_log_error("Message error: type mismatch: %d", (int)app_msg->type);
        hal_panic();
    }
    hal_rpmsg_free_rx_buffer(&RPMSG_CHANNEL, buffer);
    buffer = NULL;
    len = 0;

    hal_log_info("Enter RPMSG loop");

    for (;;) {
        // Receive message
        hal_assert(hal_rpmsg_recv_nocopy(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
        app_msg = (const IppAppMsg *)buffer;

        switch (app_msg->type) {
        case IPP_APP_MSG_CONNECT:
            IOC_STARTED = true;
            xSemaphoreGive(RPMSG_SEND_SEM);
            hal_log_info("MCU program is already started");
            break;

        case IPP_APP_MSG_KEEP_ALIVE:
            break;

        case IPP_APP_MSG_DOUT_SET: {
            SkifioDout mask = (SkifioDout)((1 << SKIFIO_DOUT_SIZE) - 1);
            SkifioDout value = app_msg->dout_set.value;
            if (~mask & value) {
                hal_log_warn("dout is out of bounds: %lx", (uint32_t)value);
            }
            DIO.out = value & mask;
            // hal_log_info("Dout write: 0x%lx", (uint32_t)DIO.out);
            hal_assert(skifio_dout_write(DIO.out) == HAL_SUCCESS);
            break;
        }
        case IPP_APP_MSG_DAC_WF: {
            size_t added_data_size = xStreamBufferSend(
                DAC.queue,
                app_msg->dac_wf.elements.data,
                app_msg->dac_wf.elements.len * sizeof(int32_t),
                0);
            hal_assert(added_data_size % sizeof(int32_t) == 0);
            DAC.was_set = true;

            if (added_data_size / sizeof(int32_t) != app_msg->dac_wf.elements.len) {
                hal_log_error("Not enough space in dac waveform buffer to save new data");
                ++STATS.dac_wf.buff_was_full;
            }

            DAC.waiting_for_data = false;
            break;
        }
        default:
            hal_log_error("Wrong message type: %d", (int)app_msg->type);
            continue;
        }

        hal_assert(hal_rpmsg_free_rx_buffer(&RPMSG_CHANNEL, buffer) == HAL_SUCCESS);
    }

    hal_log_error("End of task_rpmsg()");
    hal_panic();

    // FIXME: Should never reach this point - otherwise virtio hangs
    hal_assert(hal_rpmsg_destroy_channel(&RPMSG_CHANNEL) == HAL_SUCCESS);

    hal_rpmsg_deinit();
}

void rpmsg_init(Rpmsg *rpmsg, Control *control, Statistics *stats) {
    rpmsg->send_sem = xSemaphoreCreateBinary();
    hal_assert(rpmsg->send_sem != NULL);
    control_set_ready_sem(control, &rpmsg->send_sem);

    rpmsg->alive = false;

    rpmsg->control = control;
    rpmsg->stats = stats;
}

void rpmsg_deinit(Rpmsg *rpmsg) {
    xSemaphoreDelete(rpmsg->send_sem);
}

/// Start rpmsg tasks.
void rpmsg_run(Rpmsg *rpmsg);
