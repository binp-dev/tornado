#include "sync.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <fsl_common.h>
#include <fsl_iomuxc.h>
#include <fsl_gpt.h>

#include <hal/assert.h>
#include <hal/io.h>

#define SYN_10K_MUX IOMUXC_SAI3_TXC_GPIO5_IO00
// #define SYN_10K_MUX IOMUXC_SAI3_TXC_GPT1_COMPARE2
#define SYN_10K_PIN 5, 0

#define SYN_1_MUX IOMUXC_SAI3_RXD_GPIO4_IO30
// #define SYN_1_MUX IOMUXC_SAI3_RXD_GPT1_COMPARE1
#define SYN_1_PIN 4, 30

#define SYNC_GPT_IRQ_ID GPT1_IRQn
#define SYNC_GPT GPT1
#define SYNC_GPT_CLK_FREQ \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootGpt1)) \
     / (CLOCK_GetRootPostDivider(kCLOCK_RootGpt1)) / 2) /* SYSTEM PLL1 DIV2 */
#define SYNC_GPT_IRQHandler GPT1_IRQHandler

typedef struct {
    HalGpioGroup group;
    HalGpioPin pins[2];
    volatile uint32_t counter;
} Sync;

static Sync SYNC;

static void update_pins() {
    hal_gpio_pin_write(&SYNC.pins[0], SYNC.counter % 2 != 0);
}

static void gpt_init() {
    gpt_config_t gptConfig;

    CLOCK_SetRootMux(kCLOCK_RootGpt1, kCLOCK_GptRootmuxSysPll1Div2); /* Set GPT1 source to SYSTEM PLL1 DIV2 400MHZ */
    CLOCK_SetRootDivider(kCLOCK_RootGpt1, 1U, 4U); /* Set root clock to 400MHZ / 4 = 100MHZ */

    GPT_GetDefaultConfig(&gptConfig);

    /* Initialize GPT module */
    GPT_Init(SYNC_GPT, &gptConfig);

    /* Divide GPT clock source frequency by 3 inside GPT module */
    GPT_SetClockDivider(SYNC_GPT, 3);
}

void sync_init() {
    IOMUXC_SetPinMux(SYN_10K_MUX, 0u);
    IOMUXC_SetPinMux(SYN_1_MUX, 0u);

    hal_gpio_group_init(&SYNC.group);
    hal_gpio_pin_init(&SYNC.pins[0], &SYNC.group, SYN_10K_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&SYNC.pins[1], &SYNC.group, SYN_1_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_DISABLED);

    SYNC.counter = 0;
    update_pins();

    gpt_init();
    hal_print("GPT initialized");
}

void sync_deinit() {
    hal_panic();
}

extern void user_sync_intr();

static void handle_sync() {
    // Update state and pins
    SYNC.counter += 1;
    update_pins();

    if (SYNC.counter % 2 == 1) {
        user_sync_intr();
    }
}

void SYNC_GPT_IRQHandler(void) {
    /* Clear interrupt flag.*/
    GPT_ClearStatusFlags(SYNC_GPT, kGPT_OutputCompare1Flag);

    handle_sync();

/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

void sync_start(uint32_t period_us) {
    uint64_t gptPeriod;

    /* Get GPT clock frequency */
    gptPeriod = SYNC_GPT_CLK_FREQ;

    /* GPT frequency is divided by 3 inside module */
    gptPeriod /= 3;

    gptPeriod *= period_us;
    gptPeriod /= 1000000;
    gptPeriod /= 2;

    /* Set both GPT modules to 1 second duration */
    GPT_SetOutputCompareValue(SYNC_GPT, kGPT_OutputCompare_Channel1, (uint32_t)gptPeriod);

    /* Enable GPT Output Compare1 interrupt */
    GPT_EnableInterrupts(SYNC_GPT, kGPT_OutputCompare1InterruptEnable);

    /* Enable at the Interrupt */
    EnableIRQ(SYNC_GPT_IRQ_ID);

    /* Start Timer */
    GPT_StartTimer(SYNC_GPT);
}

void sync_stop() {
    GPT_StopTimer(SYNC_GPT);
    DisableIRQ(SYNC_GPT_IRQ_ID);
    GPT_DisableInterrupts(SYNC_GPT, kGPT_OutputCompare1InterruptEnable);
}
