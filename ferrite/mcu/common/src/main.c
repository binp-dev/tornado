#include <common/main.h>

#include <stdint.h>
#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <ipp.h>

#include <hal/rpmsg.h>


#define TASK_STACK_SIZE 256

static void task_rpmsg(void *param) {
    hal_rpmsg_init();

    hal_rpmsg_channel channel;
    // FIXME: Check retcode
    hal_rpmsg_create_channel(&channel, 0);

    uint8_t *buffer = NULL;
    size_t len = 0;
    hal_rpmsg_recv_nocopy(&channel, &buffer, &len, HAL_WAIT_FOREVER);

    //IppLoadStatus st;
    IppMsgAppAny app_msg;
    /*st = */ipp_msg_app_load(&app_msg, buffer, len);
    // FIXME: ASSERT(st == IPP_LOAD_OK);
    // if (msg.type == IPP_APP_START) {
    //     APP_INFO("RPMSG received start signal");
    // } else {
    //     PANIC_("RPMSG receive start signal error: { status: %d, len: %d, type: %d }", sst, msg_len, (int)msg.type);
    // }
    hal_rpmsg_free_rx_buffer(&channel, buffer);
    buffer = NULL;
    len = 0;

    // FIXME: Check retcode
    hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER);
    
    IppMsgMcuAny mcu_msg = {
        .type = IPP_MCU_DEBUG,
        .debug = {
            .message = "Start signal received",
        },
    };

    ipp_msg_mcu_store(&mcu_msg, buffer);

    // FIXME: Check retcode
    hal_rpmsg_send_nocopy(&channel, buffer, ipp_msg_mcu_len(&mcu_msg));

    // FIXME: Check retcode
    hal_rpmsg_destroy_channel(&channel);
    hal_rpmsg_deinit();
}

int common_main() {
    /* Create tasks. */
    xTaskCreate(
        task_rpmsg, "RPMSG task",
        TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    //PANIC_("End of main()");
    return 0;
}
