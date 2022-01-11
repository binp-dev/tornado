#include <board.h>

#include <stdint.h>
#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <ipp.h>

#include <hal/assert.h>
#include <hal/io.h>
#include <hal/rpmsg.h>

#include "senkov.h"


#define TASK_STACK_SIZE 256

static void task_rpmsg(void *param) {
    hal_rpmsg_init();

    hal_rpmsg_channel channel;
    hal_assert(hal_rpmsg_create_channel(&channel, 0) == HAL_SUCCESS);
#ifdef HAL_PRINT_RPMSG
    hal_io_rpmsg_init(&channel);
#endif

    uint8_t *buffer = NULL;
    size_t len = 0;
    const IppAppMsg *app_msg = NULL;

    // Wait for IOC start
    hal_assert(hal_rpmsg_recv_nocopy(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
    app_msg = (const IppAppMsg *)buffer;
    switch (app_msg->type) {
    case IPP_APP_MSG_START:
        hal_log_info("Start message received");
        break;
    default:
        hal_log_error("Wrong start message type: %d", (int)app_msg->type);
        hal_panic();
    }
    hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);

    hal_log_info("Senkov driver init");
    hal_assert(senkov_init() == HAL_SUCCESS);

    for (;;) {
        uint32_t value = 0;
        uint8_t index = 0;

        // Receive message
        hal_assert(hal_rpmsg_recv_nocopy(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
        app_msg = (const IppAppMsg *)buffer;
        switch (app_msg->type) {
        case IPP_APP_MSG_DAC_SET:
            value = ipp_uint24_load(app_msg->dac_set.value);
            hal_log_info("Write DAC value: 0x%06lx", value);
            hal_assert(senkov_write_dac(value) == HAL_SUCCESS);
            break;

        case IPP_APP_MSG_ADC_REQ:
            index = app_msg->adc_req.index;
            hal_assert(senkov_read_adc(index, &value) == HAL_SUCCESS);
            hal_log_info("Read ADC%d value: 0x%06lx", (int)index, value);

            hal_assert(hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
            IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
            mcu_msg->type = IPP_MCU_MSG_ADC_VAL;
            mcu_msg->adc_val.index = index;
            mcu_msg->adc_val.value = ipp_uint24_store(value);
            hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);

            break;

        default:
            hal_log_error("Wrong message type: %d", (int)app_msg->type);
            hal_panic();
        }
        hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
        vTaskDelay(1);
    }

    hal_log_error("End of task_rpmsg()");
    hal_panic();

    hal_assert(senkov_deinit() == HAL_SUCCESS);

    /* FIXME: Should never reach this point - otherwise virtio hangs */
    hal_assert(hal_rpmsg_destroy_channel(&channel) == HAL_SUCCESS);
    
    hal_rpmsg_deinit();
}

int main(void) {
    BOARD_RdcInit();
    BOARD_ClockInit();

    /* Create tasks. */
    xTaskCreate(
        task_rpmsg, "RPMSG task",
        TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    hal_log_error("End of main()");
    hal_panic();

    return 0;
}
