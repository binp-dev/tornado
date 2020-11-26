#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "app_debug.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app.h"
#include "app_flexcan.h"
#include "app_gpt.h"
#include "app_rpmsg.h"
#include "app_time.h"
#include "app_log.h"
#include "app_gpio.h"

#include "../../common/proto.h"


#define APP_TASK_STACK_SIZE    256
#define APP_RPMSG_BUF_SIZE     256

//static uint8_t buffer[APP_RPMSG_BUF_SIZE] = {0};


static void APP_Task_Rpmsg(void *param) {
    APP_INFO("RPMSG task start");

    if (APP_RPMSG_Init() != 0) {
        PANIC_("RPMSG init error");
    }

#ifdef APP_DEBUG_IO_RPMSG
    APP_Debug_IO_RPMSG_Enable();
#endif // APP_DEBUG_IO_RPMSG

    /* Initialize GPIO */
    APP_GPIO_HardwareInit();

    SemaphoreHandle_t clock_sem = xSemaphoreCreateBinary();
    ASSERT(clock_sem);

    //APP_INFO("Start GPT");
    //APP_GPT_Init(APP_GPT_SEC, clock_sem);

    APP_INFO("Init GPIO");
    ASSERT(APP_GPIO_Init(APP_GPIO_MODE_INPUT, clock_sem) == 0);

    uint8_t ssid;
    uint32_t slen = 0;
    int32_t sst = APP_RPMSG_Receive(&ssid, &slen, 1, APP_FOREVER_MS);
    if (sst == 0 && slen == 1 && ssid == PSCA_START) {
        APP_INFO("RPMSG received start signal");
    } else {
        PANIC_("RPMSG receive start signal error: { status: %d, len: %d, sid: %d }", sst, slen, (int)ssid);
    }

    const int INTR_DIV = 1000;
    int counter = 0;
    while (true) {
        for (int i = 0; i < INTR_DIV; ++i) {
            ASSERT(xSemaphoreTake(clock_sem, portMAX_DELAY) == pdTRUE);
        }
        APP_INFO("Clock: %d bunch of %d interrupts received!", counter, INTR_DIV);
        counter += 1;
    }

    /*
    while(true) {
        uint32_t len = 0;
        // FIXME: Avoid buffer switching while receiving data.
        int32_t status = APP_RPMSG_Receive(buffer, &len, APP_RPMSG_BUF_SIZE, APP_FOREVER_MS);
        APP_INFO("RPMSG receive status: %d", status);
        if (status == 0) {
            APP_INFO("RPMSG mesage received: { len: %d, sid: %d }", len, (int)buffer[0]);
            PRINTF("[INFO] RPMSG: [%d] ", len);
            for (uint32_t i = 0; i < len; ++i) {
                PRINTF("%02X ", buffer[i]);
            }
            PRINTF("\r\n");
        } else {
            PANIC_("RPMSG receive error");
        }
    }
    */

    APP_RPMSG_Deinit();
}

int main(void) {
    /* Initialize board specified hardware. */
    hardware_init();

    PRINTF("\r\n\r\n");
    APP_INFO("Program start");
    
    /* Create tasks. */
    xTaskCreate(
        APP_Task_Rpmsg, "RPMSG task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );


    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    PANIC_("End of main()");
    return 0;
}
