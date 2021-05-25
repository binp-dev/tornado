#include <stdint.h>
#include <stdbool.h>
#include <board.h>
#include <gpt.h>
#include <hal/defs.h>
#include <hal/gpt.h>


#define HAL_GPT_IRQ_PRIORITY 3

static SemaphoreHandle_t targets[HAL_GPT_INSTANCE_COUNT] = {NULL};

/*! @brief GPT interrupt handlers. */
void BOARD_GPTA_HANDLER();
void BOARD_GPTB_HANDLER();


hal_retcode hal_gpt_init(uint32_t instance) {
    enum _rdc_pdap pdap;
    enum _ccm_ccgr_gate ccm_ccgr;
    enum _ccm_root_control ccm_root;
    switch (instance) {
    case 0:
        pdap = BOARD_GPTA_RDC_PDAP;
        ccm_ccgr = BOARD_GPTA_CCM_CCGR;
        ccm_root = BOARD_GPTA_CCM_ROOT;
        break;
    case 1:
        pdap = BOARD_GPTB_RDC_PDAP;
        ccm_ccgr = BOARD_GPTB_CCM_CCGR;
        ccm_root = BOARD_GPTB_CCM_ROOT;
        break;
    default:
        return HAL_OUT_OF_BOUNDS;
    }

    /* In this example, we need to grasp board GPT exclusively */
    RDC_SetPdapAccess(RDC, pdap, 3 << (BOARD_DOMAIN_ID * 2), false, false);

    /* Select GPTx clock derived from OSC 24M */
    CCM_UpdateRoot(CCM, ccm_root, ccmRootmuxGptOsc24m, 0, 0);

    /* Enable clock used by GPTx */
    CCM_EnableRoot(CCM, ccm_root);
    CCM_ControlGate(CCM, ccm_ccgr, ccmClockNeededRunWait);

    return HAL_SUCCESS;
}

hal_retcode hal_gpt_deinit(uint32_t instance) {
    //! FIXME: Implement deinitialization.
    return HAL_UNIMPLEMENTED;
}

hal_retcode hal_gpt_start(uint32_t instance, uint32_t period, SemaphoreHandle_t target) {
    gpt_init_config_t config = {
        .freeRun     = false,
        .waitEnable  = true,
        .stopEnable  = true,
        .dozeEnable  = true,
        .dbgEnable   = false,
        .enableMode  = true
    };

    GPT_Type *baseaddr;
    enum IRQn irqn;
    enum _gpt_output_compare_channel cmpch;
    enum _gpt_status_flag sflag;
    switch (instance) {
    case 0:
        baseaddr = BOARD_GPTA_BASEADDR;
        irqn = BOARD_GPTA_IRQ_NUM;
        cmpch = gptOutputCompareChannel1;
        sflag = gptStatusFlagOutputCompare1;
        break;
    case 1:
        baseaddr = BOARD_GPTB_BASEADDR;
        irqn = BOARD_GPTB_IRQ_NUM;
        cmpch = gptOutputCompareChannel2;
        sflag = gptStatusFlagOutputCompare2;
        break;
    default:
        return HAL_OUT_OF_BOUNDS;
    }

    /* Initialize GPT module */
    GPT_Init(baseaddr, &config);
    /* Set GPT clock source */
    GPT_SetClockSource(baseaddr, gptClockSourceOsc);
    /* Divide GPTA osc clock source frequency by 2, and divide additional 2 inside GPT module  */
    GPT_SetOscPrescaler(baseaddr, 1);
    GPT_SetPrescaler(baseaddr, 1);

    targets[instance] = target;

    /* Get GPT clock frequency */
    //period = 24000000/4; /* GPT is bound to OSC directly, with OSC divider 2 */
    /* Set GPT modules to specified duration */
    GPT_SetOutputCompareValue(baseaddr, cmpch, period);
    /* Set GPT interrupt priority to same value to avoid handler preemption */
    NVIC_SetPriority(irqn, HAL_GPT_IRQ_PRIORITY);
    /* Enable NVIC interrupt */
    NVIC_EnableIRQ(irqn);
    /* Enable GPT Output Compare1 interrupt */
    GPT_SetIntCmd(baseaddr, sflag, true);
    /* GPT start */
    GPT_Enable(baseaddr);

    return HAL_SUCCESS;
}

hal_retcode hal_gpt_stop(uint32_t instance) {
    GPT_Type *baseaddr;
    enum IRQn irqn;
    switch (instance) {
    case 0:
        baseaddr = BOARD_GPTA_BASEADDR;
        irqn = BOARD_GPTA_IRQ_NUM;
        break;
    case 1:
        baseaddr = BOARD_GPTB_BASEADDR;
        irqn = BOARD_GPTB_IRQ_NUM;
        break;
    default:
        return HAL_OUT_OF_BOUNDS;
    }

    NVIC_DisableIRQ(irqn);
    GPT_Disable(baseaddr);
    return HAL_SUCCESS;
}


static void handle_gpt(uint32_t instance) {
    if (targets[instance] != NULL) {
        BaseType_t hptw = pdFALSE;

        /* Notify target task */
        xSemaphoreGiveFromISR(targets[instance], &hptw);

        /* Yield to higher priority task */
        portYIELD_FROM_ISR(hptw);
    }
}

void BOARD_GPTA_HANDLER() {
    GPT_ClearStatusFlag(BOARD_GPTA_BASEADDR, gptStatusFlagOutputCompare1);
    handle_gpt(0);
}

void BOARD_GPTB_HANDLER() {
    GPT_ClearStatusFlag(BOARD_GPTB_BASEADDR, gptStatusFlagOutputCompare2);
    handle_gpt(1);
}
