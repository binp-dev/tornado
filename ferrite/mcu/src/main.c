#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "debug_console_imx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app.h"
#include "app_flexcan.h"
#include "app_gpt.h"
#include "app_rpmsg.h"
#include "app_time.h"
#include "app_log.h"

#include "../../common/proto.h"


#define APP_TASK_STACK_SIZE    256
#define APP_RPMSG_BUF_SIZE     256

static SemaphoreHandle_t wf_send_sem = NULL;
static SemaphoreHandle_t wf_ready_sem = NULL;

#define APP_WF_BUF_SIZE        APP_RPMSG_BUF_SIZE
#define APP_WF_OFFSET          1
#define APP_WF_POINT_SIZE      3

static uint8_t wf_buffers[2][APP_WF_BUF_SIZE] = {{0}, {0}};
static volatile uint8_t wf_current = 0;


static void APP_Task_FlexcanSend(void *param) {
    APP_FLEXCAN_Frame frame = {
        .id = 0x123,
        .len = 8,
        .data = {0}
    };
    uint8_t *buf = NULL;
    size_t pos = 0;

    APP_INFO("FLEXCAN task start");

    while (true) {
        if (buf == NULL || pos + APP_WF_POINT_SIZE > APP_WF_BUF_SIZE) {
            APP_INFO("RPMSG send waveform request");
            // FIXME: Use `psc-common`.
            uint8_t sid = PSCM_WF_REQ;
            APP_RPMSG_Send(&sid, 1);

            ASSERT(xSemaphoreTake(wf_ready_sem, portMAX_DELAY) == pdTRUE);
            
            wf_current = !wf_current;
            buf = wf_buffers[wf_current];
            pos = APP_WF_OFFSET;
        }

        frame.len = APP_WF_POINT_SIZE;
        memcpy(frame.data, buf + pos, APP_WF_POINT_SIZE);
        pos += APP_WF_POINT_SIZE;

        ASSERT(xSemaphoreTake(wf_send_sem, portMAX_DELAY) == pdTRUE);

        if (APP_FLEXCAN_Send(&frame, APP_FOREVER_MS) != 0) {
            PANIC_("Cannot send CAN frame");
        }

        *(uint64_t*)&frame.data += 1;


        if (buf != NULL) {
            // TODO: What 
        }
    }
}

static void APP_Task_FlexcanReceive(void *param) {
    APP_FLEXCAN_Frame frame;

    APP_INFO("FLEXCAN receive task start");
    
    while (true) {
        if (APP_FLEXCAN_Receive(&frame, APP_FOREVER_MS) == 0) {
            PRINTF("[INFO] FLEXCAN: 0x%03X # ", frame.id);
            for (uint8_t i = 0; i < frame.len; ++i) {
                PRINTF("%0X ", frame.data[frame.len - i - 1]);
            }
            PRINTF("\r\n");
        } else {
            PANIC_("Cannot receive CAN frame");
        }
    }
}

static void APP_Task_Rpmsg(void *param) {
    APP_INFO("RPMSG task start");

    if (APP_RPMSG_Init() != 0) {
        PANIC_("RPMSG init error");
    }

    uint8_t ssid;
    uint32_t slen = 0;
    int32_t sst = APP_RPMSG_Receive(&ssid, &slen, 1, APP_FOREVER_MS);
    // FIXME: Use `psc-common`.
    if (sst == 0 && slen == 1 && ssid == PSCA_START) {
        APP_INFO("RPMSG received start signal");
    } else {
        PANIC_("RPMSG receive start signal error: { status: %d, len: %d, sid: %d }", sst, slen, (int)ssid);
    }

    xTaskCreate(
        APP_Task_FlexcanSend, "FLEXCAN task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL
    );
    xTaskCreate(
        APP_Task_FlexcanReceive, "FLEXCAN Receive task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL
    );

    while(true) {
        uint32_t len = 0;
        uint8_t *buffer = wf_buffers[!wf_current];
        // FIXME: Avoid buffer switching while receiving data.
        int32_t status = APP_RPMSG_Receive(buffer, &len, APP_RPMSG_BUF_SIZE, APP_FOREVER_MS);
        APP_INFO("RPMSG receive status: %d", status);
        if (status == 0) {
            // FIXME: Use `psc-common`.
            APP_INFO("RPMSG mesage received: { len: %d, sid: %d }", len, (int)buffer[0]);
            ASSERT(len == APP_WF_BUF_SIZE);
            ASSERT(buffer[0] == PSCA_WF_DATA);
            xSemaphoreGive(wf_ready_sem);
            PRINTF("[INFO] RPMSG: [%d] ", len);
            for (uint32_t i = 0; i < len; ++i) {
                PRINTF("%02X ", buffer[i]);
            }
            PRINTF("\r\n");
        } else {
            PANIC_("RPMSG receive error");
        }
    }

    APP_RPMSG_Deinit();
}

int main(void) {
    /* Initialize board specified hardware. */
    hardware_init();

    PRINTF("\r\n\r\n");
    APP_INFO("Program start");

    wf_send_sem = xSemaphoreCreateBinary();
    wf_ready_sem = xSemaphoreCreateBinary();

    APP_FLEXCAN_Init(APP_FLEXCAN_Baudrate_1000, 0x321);
    
    /* Create tasks. */
    xTaskCreate(
        APP_Task_Rpmsg, "RPMSG task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    APP_INFO("Start GPT");
    APP_GPT_Init(APP_GPT_SEC/10, wf_send_sem);


    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    PANIC_("End of main()");
    return 0;
}
