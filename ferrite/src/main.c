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
#include "app_flexcan.h"
#include "app_gpt.h"


static APP_FLEXCAN_Frame tx_frame = {
    .id = 0x123,
    .len = 8,
    .data = "\xEF\xCD\xAB\x89\x67\x45\x23\x01"
};

void send() {
    APP_FLEXCAN_Send(&tx_frame);
    *(uint64_t*)tx_frame.data += 1;
}

int main(void) {
    /* Initialize board specified hardware. */
    hardware_init();

    APP_FLEXCAN_Init(APP_FLEXCAN_Baudrate_1000, 0x321);
    APP_GPT_Init(APP_GPT_SEC, send);

    while (true) {
        APP_FLEXCAN_Frame rx_frame;
        if (APP_FLEXCAN_TryRecv(&rx_frame) == 0) {
            PRINTF("\r\n0x%03X # ", rx_frame.id);
            for (uint8_t i = 0; i < rx_frame.len; ++i) {
                PRINTF("%0X ", rx_frame.data[rx_frame.len - i - 1]);
            }
        }
    }
}
