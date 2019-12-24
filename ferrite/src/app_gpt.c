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
#include "gpt.h"

#include "app_gpt.h"


static void(*gpt_callback)() = NULL;

/*! @brief GPT interrupt handler. */
static void APP_GPT_Handler();

void BOARD_GPTA_HANDLER(void) {
    APP_GPT_Handler();
}

void APP_GPT_HardwareInit() {
    /* In this example, we need to grasp board GPT exclusively */
    RDC_SetPdapAccess(RDC, BOARD_GPTA_RDC_PDAP, 3 << (BOARD_DOMAIN_ID * 2), false, false);

    /* Select GPTA clock derived from OSC 24M */
    CCM_UpdateRoot(CCM, BOARD_GPTA_CCM_ROOT, ccmRootmuxGptOsc24m, 0, 0);

    /* Enable clock used by GPTA */
    CCM_EnableRoot(CCM, BOARD_GPTA_CCM_ROOT);
    CCM_ControlGate(CCM, BOARD_GPTA_CCM_CCGR, ccmClockNeededRunWait);
}

uint8_t APP_GPT_Init(uint32_t period, void (*callback)()) {
    gpt_init_config_t config = {
        .freeRun     = false,
        .waitEnable  = true,
        .stopEnable  = true,
        .dozeEnable  = true,
        .dbgEnable   = false,
        .enableMode  = true
    };

    /* Initialize GPT module */
    GPT_Init(BOARD_GPTA_BASEADDR, &config);
    /* Set GPT clock source */
    GPT_SetClockSource(BOARD_GPTA_BASEADDR, gptClockSourceOsc);
    /* Divide GPTA osc clock source frequency by 2, and divide additional 2 inside GPT module  */
    GPT_SetOscPrescaler(BOARD_GPTA_BASEADDR, 1);
    GPT_SetPrescaler(BOARD_GPTA_BASEADDR, 1);

    gpt_callback = callback;

    /* Get GPT clock frequency */
    //period = 24000000/4; /* A is bound to OSC directly, with OSC divider 2 */
    /* Set both GPT modules to 1 second duration */
    GPT_SetOutputCompareValue(BOARD_GPTA_BASEADDR, gptOutputCompareChannel1, period);
    /* Set GPT interrupt priority to same value to avoid handler preemption */
    NVIC_SetPriority(BOARD_GPTA_IRQ_NUM, 3);
    /* Enable NVIC interrupt */
    NVIC_EnableIRQ(BOARD_GPTA_IRQ_NUM);
    /* Enable GPT Output Compare1 interrupt */
    GPT_SetIntCmd(BOARD_GPTA_BASEADDR, gptStatusFlagOutputCompare1, true);
    /* GPT start */
    GPT_Enable(BOARD_GPTA_BASEADDR);

    return 0;
}

void APP_GPT_Handler() {
    GPT_ClearStatusFlag(BOARD_GPTA_BASEADDR, gptStatusFlagOutputCompare1);
    if (gpt_callback != NULL) {
        gpt_callback();
    }
}
