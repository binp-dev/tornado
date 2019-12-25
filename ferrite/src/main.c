/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "debug_console_imx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app_flexcan.h"
#include "app_gpt.h"
#include "app_log.h"


#define APP_TASK_STACK_SIZE 256

static SemaphoreHandle_t send_semaphore = NULL;


static void APP_Task_FlexcanSend(void *param) {
    APP_FLEXCAN_Frame frame = {
        .id = 0x123,
        .len = 8,
        .data = "\xEF\xCD\xAB\x89\x67\x45\x23\x01"
    };

    while (true) {
        if (xSemaphoreTake(send_semaphore, portMAX_DELAY) != pdTRUE) {
            APP_ERROR("FLEXCAN Send task notify timed out (unreachable)");
        }
        if (APP_FLEXCAN_Send(&frame, 0) != 0) {
            APP_ERROR("Cannot send CAN frame");
        }
        *(uint64_t*)frame.data += 1;
    }
}

static void APP_Task_FlexcanReceive(void *param) {
    APP_FLEXCAN_Frame frame;
    
    while (true) {
        if (APP_FLEXCAN_Receive(&frame, 0) == 0) {
            PRINTF("0x%03X # ", frame.id);
            for (uint8_t i = 0; i < frame.len; ++i) {
                PRINTF("%0X ", frame.data[frame.len - i - 1]);
            }
            PRINTF("\r\n");
        } else {
            APP_ERROR("Cannot receive CAN frame");
        }
    }
}

int main(void) {
    /* Initialize board specified hardware. */
    hardware_init();

    APP_FLEXCAN_Init(APP_FLEXCAN_Baudrate_1000, 0x321);

    send_semaphore = xSemaphoreCreateBinary();
    
    /* Create tasks. */
    xTaskCreate(
        APP_Task_FlexcanReceive, "FLEXCAN Receive task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL
    );

    xTaskCreate(
        APP_Task_FlexcanSend, "FLEXCAN Send task",
        APP_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    APP_GPT_Init(APP_GPT_SEC/10, send_semaphore);

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    vSemaphoreDelete(send_semaphore);

    /* Should never reach this point. */
    while(true);
}
