/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define OUT_GPIO     GPIO5
#define OUT_GPIO_PIN 27U
#define IN_GPIO     GPIO5
#define IN_GPIO_PIN 26U

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* The PIN status */
volatile bool g_pinSet = false;
volatile bool g_interrupted = false;
/*******************************************************************************
 * Code
 ******************************************************************************/
void GPIO5_Combined_16_31_IRQHandler() {
    g_interrupted = true;
    GPIO_ClearPinsInterruptFlags(IN_GPIO, 1 << IN_GPIO_PIN);

    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    //__DSB();
#endif
}

/*!
 * @brief Main function
 */
int main(void)
{
    /* Define the init structure for the output LED pin*/
    gpio_pin_config_t out_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    gpio_pin_config_t in_config = {kGPIO_DigitalInput, 0, kGPIO_IntRisingEdge};

    /* Board pin, clock, debug console init */
    /* M7 has its local cache and enabled by default,
     * need to set smart subsystems (0x28000000 ~ 0x3FFFFFFF)
     * non-cacheable before accessing this address region */
    BOARD_InitMemory();

    /* Board specific RDC settings */
    BOARD_RdcInit();

    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    /* Print a note to terminal. */
    PRINTF("GPIO Driver example\r\n");
    PRINTF("The LED is blinking.\r\n");

    /* Init output LED GPIO. */
    GPIO_PinInit(OUT_GPIO, OUT_GPIO_PIN, &out_config);
    GPIO_PinInit(IN_GPIO, IN_GPIO_PIN, &in_config);
    GPIO_ClearPinsInterruptFlags(IN_GPIO, 1 << IN_GPIO_PIN);
    GPIO_EnableInterrupts(IN_GPIO, 1 << IN_GPIO_PIN);
    EnableIRQ(GPIO5_Combined_16_31_IRQn);

    PRINTF("IRQ Enabled.\r\n");

    int counter = 0;
    while (1)
    {

        SDK_DelayAtLeastUs(1000000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
#if (defined(FSL_FEATURE_IGPIO_HAS_DR_TOGGLE) && (FSL_FEATURE_IGPIO_HAS_DR_TOGGLE == 1))
        GPIO_PortToggle(OUT_GPIO, 1u << OUT_GPIO_PIN);
#else
        if (g_pinSet)
        {
            GPIO_PinWrite(OUT_GPIO, OUT_GPIO_PIN, 0U);
            g_pinSet = false;
        }
        else
        {
            GPIO_PinWrite(OUT_GPIO, OUT_GPIO_PIN, 1U);
            g_pinSet = true;
        }
#endif /* FSL_FEATURE_IGPIO_HAS_DR_TOGGLE */

        PRINTF("%d input is %d.\r\n", counter, (int)GPIO_PinRead(IN_GPIO, IN_GPIO_PIN));
        PRINTF("ISR is %x.\r\n", IN_GPIO->ISR);
        PRINTF("IMR is %x.\r\n", IN_GPIO->IMR);

        if (g_interrupted) {
            g_interrupted = false;
            PRINTF("INTERRUPTED!\r\n");
        }
        counter += 1;
    }
}
