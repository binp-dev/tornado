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

#include "app_log.h"


#define APP_TASK_STACK_SIZE 256
#define APP_WAIT_FOREVER 0xFFFFFFFFul


static SemaphoreHandle_t send_semaphore = NULL;

#define RPMSG_BUFSIZE 256
static uint8_t buffer[RPMSG_BUFSIZE] = {0};


static void APP_Task_Flexcan(void *param) {
    APP_FLEXCAN_Frame frame = {
        .id = 0x123,
        .len = 8,
        .data = "\xEF\xCD\xAB\x89\x67\x45\x23\x01"
    };

    APP_INFO("FLEXCAN task start");

    while (true) {
        if (xSemaphoreTake(send_semaphore, portMAX_DELAY) != pdTRUE) {
            PANIC("FLEXCAN Send task notify timed out");
        }
        
        if (APP_FLEXCAN_Send(&frame, APP_WAIT_FOREVER) != 0) {
            PANIC("Cannot send CAN frame");
        }

        *(uint64_t*)&frame.data += 1;
    }
}

static void APP_Task_FlexcanReceive(void *param) {
    APP_FLEXCAN_Frame frame;

    APP_INFO("FLEXCAN receive task start");
    
    while (true) {
        if (APP_FLEXCAN_Receive(&frame, APP_WAIT_FOREVER) == 0) {
            PRINTF("[INFO] FLEXCAN: 0x%03X # ", frame.id);
            for (uint8_t i = 0; i < frame.len; ++i) {
                PRINTF("%0X ", frame.data[frame.len - i - 1]);
            }
            PRINTF("\r\n");
        } else {
            PANIC("Cannot receive CAN frame");
        }
    }
}

static void APP_Task_Rpmsg(void *param) {
    APP_INFO("RPMSG task start");

    if (APP_RPMSG_Init() != 0) {
        APP_ERROR("RPMSG init error");
        return;
    }

    while(true) {
        uint32_t len = 0;
        int32_t status = APP_RPMSG_Receive(buffer, &len, RPMSG_BUFSIZE, APP_WAIT_FOREVER);
        APP_INFO("RPMSG receive status: %d", status);
        if (status == 0) {
            PRINTF("[INFO] RPMSG: [%d] ", len);
            for (uint8_t i = 0; i < len; ++i) {
                PRINTF("%02X ", buffer[i]);
            }
            PRINTF("\r\n");
        } else {
            APP_ERROR("RPMSG receive error");
        }
    }

    APP_RPMSG_Deinit();
}

int main(void) {
    /* Initialize board specified hardware. */
    hardware_init();

    PRINTF("\r\n\r\n");
    APP_INFO("Program start");

    send_semaphore = xSemaphoreCreateBinary();

    APP_FLEXCAN_Init(APP_FLEXCAN_Baudrate_1000, 0x321);
    
    /* Create tasks. */
    xTaskCreate(
        APP_Task_Flexcan, "FLEXCAN task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL
    );

    xTaskCreate(
        APP_Task_Rpmsg, "RPMSG task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    xTaskCreate(
        APP_Task_FlexcanReceive, "FLEXCAN Receive task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL
    );    

    APP_INFO("Start GPT");
    APP_GPT_Init(APP_GPT_SEC/10, send_semaphore);


    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    PANIC("Unreachable");
    return 0;
}
