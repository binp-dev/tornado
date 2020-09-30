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

#include <stdlib.h>
#include <stdint.h>

#include "board.h"
#include "debug_console_imx.h"
#include "pin_mux.h"
#include "gpio_pins.h"
#include "gpio_imx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app.h"
#include "app_log.h"
#include "app_gpio.h"



static uint8_t app_gpio_mode = 0;

static gpio_config_t app_gpio_config = {
    "App GPIO",                          /* name */
    &IOMUXC_SW_MUX_CTL_PAD_EPDC_DATA04,  /* muxReg */
    5,                                   /* muxConfig */
    &IOMUXC_SW_PAD_CTL_PAD_EPDC_DATA04,  /* padReg */
    /*
    IOMUXC_SW_PAD_CTL_PAD_I2C2_SDA_PS(2) |
    IOMUXC_SW_PAD_CTL_PAD_I2C2_SDA_PE_MASK |
    IOMUXC_SW_PAD_CTL_PAD_I2C2_SDA_HYS_MASK,
    */
    IOMUXC_SW_PAD_CTL_PAD_EPDC_DATA04_PS(0) | IOMUXC_SW_PAD_CTL_PAD_EPDC_DATA04_PE_MASK, /* padConfig */
    GPIO2,                               /* base */
    4                                    /* pin */
};

/*
#define APP_GPIO_IRQ_NUM BOARD_GPIO_KEY_IRQ_NUM
#define APP_GPIO_HANDLER BOARD_GPIO_KEY_HANDLER
#define APP_GPIO_CONFIG BOARD_GPIO_KEY_CONFIG
*/

#define APP_GPIO_IRQ_NUM GPIO2_INT15_0_IRQn
#define APP_GPIO_HANDLER GPIO2_INT15_0_Handler
#define APP_GPIO_CONFIG (&app_gpio_config)

static volatile SemaphoreHandle_t gpio_sem = NULL;

void APP_GPIO_HANDLER();

void APP_GPIO_HardwareInit() {
    /* In this demo, we need to share board GPIO, we can set sreq argument to true
     * when the peer core could also access GPIO with RDC_SEMAPHORE, or the peer
     * core doesn't access the GPIO at all */
    //RDC_SetPdapAccess(RDC, rdcPdapGpio2, 0xFF, false/*true*/, false);

    /* Enable gpio clock gate, led and key share same CCGR on this board */
    //CCM_ControlGate(CCM, ccmCcgrGateGpio2, ccmClockNeededRunWait);

    /* Configure gpio pin IOMUX */
    configure_gpio_pin(APP_GPIO_CONFIG);
}

uint8_t APP_GPIO_Init(uint32_t mode, SemaphoreHandle_t sem) {
    app_gpio_mode = mode;
    
    gpio_init_config_t pinInit = {};
    pinInit.pin = APP_GPIO_CONFIG->pin;

    if (app_gpio_mode == APP_GPIO_MODE_INPUT) {
        pinInit.direction = gpioDigitalInput;
        pinInit.interruptMode = gpioIntFallingEdge;
    } else if (app_gpio_mode == APP_GPIO_MODE_OUTPUT) {
        pinInit.direction = gpioDigitalOutput;
        pinInit.interruptMode = gpioNoIntmode;
    } else {
        PANIC_("Unknown GPIO mode: %d", app_gpio_mode);
    }

    GPIO_Init(APP_GPIO_CONFIG->base, &pinInit);

    if (mode == APP_GPIO_MODE_INPUT) {
        gpio_sem = sem;

        /* Clear the interrupt state */
        GPIO_ClearStatusFlag(APP_GPIO_CONFIG->base, APP_GPIO_CONFIG->pin);
        /* Enable GPIO pin interrupt */
        GPIO_SetPinIntMode(APP_GPIO_CONFIG->base, APP_GPIO_CONFIG->pin, true);

        /* Set GPIO interrupt priority */
        NVIC_SetPriority(APP_GPIO_IRQ_NUM, APP_GPIO_IRQ_PRIORITY);
        /* Enable the IRQ. */
        NVIC_EnableIRQ(APP_GPIO_IRQ_NUM);
    }

    return 0;
}

uint8_t APP_GPIO_Set(uint8_t on) {
    if (app_gpio_mode != APP_GPIO_MODE_OUTPUT) {
        return 1;
    }
    GPIO_WritePinOutput(APP_GPIO_CONFIG->base, APP_GPIO_CONFIG->pin, on ? gpioPinSet : gpioPinClear);
    return 0;
}

void APP_GPIO_HANDLER() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Clear the interrupt state */
    GPIO_ClearStatusFlag(APP_GPIO_CONFIG->base, APP_GPIO_CONFIG->pin);

    //APP_INFO("APP_GPIO_HANDLER()");

    /* Unlock the task to process the event. */
    xSemaphoreGiveFromISR(gpio_sem, &xHigherPriorityTaskWoken);

    /* Perform a context switch to wake the higher priority task. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
