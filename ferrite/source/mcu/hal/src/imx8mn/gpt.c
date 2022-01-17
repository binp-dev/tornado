#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "fsl_common.h"
#include "fsl_gpt.h"

#include <hal/defs.h>
#include <hal/gpt.h>
#include <hal/assert.h>

#define HAL_GPT_IRQ_PRIORITY 3

static uint32_t gpt_period_1s(clock_root_control_t root_clk) {
    // SYSTEM PLL1 DIV2
    uint32_t freq = CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl);
    freq /= CLOCK_GetRootPreDivider(root_clk);
    freq /= CLOCK_GetRootPostDivider(root_clk);
    freq /= 2;
    return freq;
}

static const _HalGptDevice DEVICES[HAL_GPT_INSTANCE_COUNT] = {
    {GPT1, kCLOCK_RootGpt1, GPT1_IRQn},
    {GPT2, kCLOCK_RootGpt2, GPT2_IRQn},
    {GPT3, kCLOCK_RootGpt3, GPT3_IRQn},
};
static const _HalGptChannel CHANNELS[HAL_GPT_CHANNELS_COUNT] = {
    {kGPT_OutputCompare_Channel1, kGPT_OutputCompare1InterruptEnable, kGPT_OutputCompare1Flag},
    {kGPT_OutputCompare_Channel2, kGPT_OutputCompare2InterruptEnable, kGPT_OutputCompare2Flag},
    {kGPT_OutputCompare_Channel3, kGPT_OutputCompare3InterruptEnable, kGPT_OutputCompare3Flag},
};

static HalGpt *INSTANCES[HAL_GPT_INSTANCE_COUNT] = {NULL};

#define GPT_IRQ_HANDLER_DECL(N) void GPT##N##_IRQHandler();

//! @brief GPT interrupt handlers.
GPT_IRQ_HANDLER_DECL(1)
GPT_IRQ_HANDLER_DECL(2)
GPT_IRQ_HANDLER_DECL(3)

hal_retcode hal_gpt_init(HalGpt *gpt, uint32_t instance) {
    if (instance < 1 || instance > HAL_GPT_INSTANCE_COUNT) {
        return HAL_OUT_OF_BOUNDS;
    }
    size_t index = instance - 1;
    if (INSTANCES[index] != NULL) {
        return HAL_FAILURE;
    }

    gpt->index = index;
    gpt->device = &DEVICES[index];
    gpt->channel = NULL;
    gpt->callbacks = NULL;
    gpt->user_data = NULL;

    // Set GPT1 source to SYSTEM PLL1 DIV2 400MHZ
    CLOCK_SetRootMux(gpt->device->root_clk, kCLOCK_GptRootmuxSysPll1Div2);
    // Set root clock to 400MHZ / 4 = 100MHZ
    CLOCK_SetRootDivider(gpt->device->root_clk, 1U, 4U);

    gpt_config_t gpt_config;
    GPT_GetDefaultConfig(&gpt_config);

    // Initialize GPT module
    GPT_Init(gpt->device->base, &gpt_config);

    // Divide GPT clock source frequency by 3 inside GPT module
    GPT_SetClockDivider(gpt->device->base, 3);

    INSTANCES[index] = gpt;
    return HAL_SUCCESS;
}

hal_retcode hal_gpt_deinit(HalGpt *gpt) {
    INSTANCES[gpt->index] = NULL;

    //! FIXME: Implement deinitialization.
    return HAL_UNIMPLEMENTED;
}

hal_retcode hal_gpt_start(
    HalGpt *gpt,
    uint32_t channel,
    uint32_t period_us,
    void (*callback)(void *),
    void *user_data //
) {
    if (channel < 1 || channel > HAL_GPT_CHANNELS_COUNT) {
        return HAL_OUT_OF_BOUNDS;
    }

    gpt->channel = &CHANNELS[channel - 1];
    gpt->callbacks = callback;
    gpt->user_data = user_data;

    // Get GPT clock frequency
    uint32_t gpt_period = gpt_period_1s(gpt->device->root_clk);

    // GPT frequency is divided by 3 inside module
    gpt_period /= 3;

    uint64_t period = ((uint64_t)gpt_period * period_us) / 1000000;
    if (period >= (1ull << 32)) {
        // Period is too long, counter will overflow.
        return HAL_OUT_OF_BOUNDS;
    }

    // Set both GPT modules to 1 second duration
    GPT_SetOutputCompareValue(gpt->device->base, gpt->channel->number, (uint32_t)period);

    if (callback != NULL) {
        // Enable GPT Output Compare1 interrupt
        GPT_EnableInterrupts(gpt->device->base, gpt->channel->intr_mask);

        NVIC_SetPriority(gpt->device->irqn, HAL_GPT_IRQ_PRIORITY);
        // Enable NVIC interrupt
        NVIC_EnableIRQ(gpt->device->irqn);
    }

    // Start Timer
    GPT_StartTimer(gpt->device->base);

    return HAL_SUCCESS;
}

hal_retcode hal_gpt_stop(HalGpt *gpt) {
    GPT_StopTimer(gpt->device->base);

    NVIC_DisableIRQ(gpt->device->irqn);

    return HAL_SUCCESS;
}

static void handle_gpt(size_t index) {
    HalGpt *gpt = INSTANCES[index];
    hal_assert(gpt != NULL);

    // Clear interrupt flag.
    GPT_ClearStatusFlags(gpt->device->base, gpt->channel->flag);

    gpt->callbacks(gpt->user_data);

    // Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
    // exception return operation might vector to incorrect interrupt
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

#define GPT_IRQ_HANDLER_DEF(N) \
    void GPT##N##_IRQHandler() { \
        handle_gpt(N - 1); \
    }

GPT_IRQ_HANDLER_DEF(1)
GPT_IRQ_HANDLER_DEF(2)
GPT_IRQ_HANDLER_DEF(3)
