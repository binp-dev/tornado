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

static GPT_Type *const gpt_bases[HAL_GPT_INSTANCE_COUNT] = {
    GPT1,
    GPT2,
    GPT3
};
static const IRQn_Type gpt_irqns[HAL_GPT_INSTANCE_COUNT] = {
    GPT1_IRQn,
    GPT2_IRQn,
    GPT3_IRQn
};
static volatile gpt_status_flag_t gpt_states[HAL_GPT_INSTANCE_COUNT] = {
    kGPT_OutputCompare_Channel1,
    kGPT_OutputCompare_Channel2,
    kGPT_OutputCompare_Channel3
};

static SemaphoreHandle_t gpt_semaphores[HAL_GPT_INSTANCE_COUNT] = {NULL};

#define GPT_IRQ_HANDLER_DECL(N) void GPT##N##_IRQHandler();

//! @brief GPT interrupt handlers.
GPT_IRQ_HANDLER_DECL(1)
GPT_IRQ_HANDLER_DECL(2)
GPT_IRQ_HANDLER_DECL(3)

hal_retcode hal_gpt_init(uint32_t instance) {
    if (instance >= HAL_GPT_INSTANCE_COUNT) {
        return HAL_OUT_OF_BOUNDS;
    }

    GPT_Type *gpt_base = gpt_bases[instance];

    clock_root_control_t root_clk;
    switch (instance + 1) {
    case 1:
        root_clk = kCLOCK_RootGpt1;
        break;
    case 2:
        root_clk = kCLOCK_RootGpt2;
        break;
    case 3:
        root_clk = kCLOCK_RootGpt3;
        break;
    default:
        hal_unreachable();
    }

    // Set GPT1 source to SYSTEM PLL1 DIV2 400MHZ
    CLOCK_SetRootMux(root_clk, kCLOCK_GptRootmuxSysPll1Div2);
    // Set root clock to 400MHZ / 4 = 100MHZ
    CLOCK_SetRootDivider(root_clk, 1U, 4U);

    gpt_config_t gpt_config;
    GPT_GetDefaultConfig(&gpt_config);

    // Initialize GPT module
    GPT_Init(gpt_base, &gpt_config);

    // Divide GPT clock source frequency by 3 inside GPT module
    GPT_SetClockDivider(gpt_base, 3);

    return HAL_SUCCESS;
}

hal_retcode hal_gpt_deinit(uint32_t instance) {
    //! FIXME: Implement deinitialization.
    return HAL_UNIMPLEMENTED;
}

hal_retcode hal_gpt_start(uint32_t instance, uint32_t period_us, SemaphoreHandle_t target) {
    if (instance >= HAL_GPT_INSTANCE_COUNT) {
        return HAL_OUT_OF_BOUNDS;
    }

    GPT_Type *gpt_base = gpt_bases[instance];
    IRQn_Type gpt_irqn = gpt_irqns[instance];

    clock_root_control_t root_clk;
    gpt_interrupt_enable_t intr_mask;
    switch (instance + 1) {
    case 1:
        root_clk = kCLOCK_RootGpt1;
        intr_mask = kGPT_OutputCompare1InterruptEnable;
        break;
    case 2:
        root_clk = kCLOCK_RootGpt2;
        intr_mask = kGPT_OutputCompare2InterruptEnable;
        break;
    case 3:
        root_clk = kCLOCK_RootGpt3;
        intr_mask = kGPT_OutputCompare3InterruptEnable;
        break;
    default:
        hal_unreachable();
    }

    gpt_semaphores[instance] = target;

    // Get GPT clock frequency
    uint32_t gpt_period = gpt_period_1s(root_clk);

    // GPT frequency is divided by 3 inside module
    gpt_period /= 3;

    uint64_t period = ((uint64_t)gpt_period * period_us) / 1000000;
    if (period >= (1ull << 32)) {
        // Period is too long, counter will overflow.
        return HAL_OUT_OF_BOUNDS;
    }

    // Set both GPT modules to 1 second duration
    GPT_SetOutputCompareValue(gpt_base, gpt_states[instance], (uint32_t)period);

    // Enable GPT Output Compare1 interrupt
    GPT_EnableInterrupts(gpt_base, intr_mask);

    NVIC_SetPriority(gpt_irqn, HAL_GPT_IRQ_PRIORITY);
    // Enable NVIC interrupt
    NVIC_EnableIRQ(gpt_irqn);

    // Start Timer
    GPT_StartTimer(gpt_base);

    return HAL_SUCCESS;
}

hal_retcode hal_gpt_stop(uint32_t instance) {
    if (instance >= HAL_GPT_INSTANCE_COUNT) {
        return HAL_OUT_OF_BOUNDS;
    }

    GPT_Type *gpt_base = gpt_bases[instance];
    IRQn_Type gpt_irqn = gpt_irqns[instance];

    GPT_StopTimer(gpt_base);

    NVIC_DisableIRQ(gpt_irqn);

    return HAL_SUCCESS;
}


static void handle_gpt(uint32_t instance) {
    if (gpt_semaphores[instance] != NULL) {
        BaseType_t hptw = pdFALSE;

        // Notify target task
        xSemaphoreGiveFromISR(gpt_semaphores[instance], &hptw);

        // Yield to higher priority task
        portYIELD_FROM_ISR(hptw);
    }
}

static void hal_dsb() {
    // Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
    // exception return operation might vector to incorrect interrupt
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

#define GPT_IRQ_HANDLER_DEF(N) \
    void GPT##N##_IRQHandler() { \
        /* Clear interrupt flag. */ \
        GPT_ClearStatusFlags(GPT##N, kGPT_OutputCompare1Flag); \
        handle_gpt(N - 1); \
        hal_dsb(); \
    }

GPT_IRQ_HANDLER_DEF(1)
GPT_IRQ_HANDLER_DEF(2)
GPT_IRQ_HANDLER_DEF(3)
