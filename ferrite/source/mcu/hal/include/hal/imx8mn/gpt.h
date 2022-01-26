#pragma once

#if !defined(HAL_IMX8MN)
#error "This header should be included only when building for i.MX7"
#endif

#include "fsl_gpt.h"

#include "FreeRTOS.h"
#include "semphr.h"

/*! @brief Available GPT instances count. */
#define HAL_GPT_INSTANCE_COUNT 3
#define HAL_GPT_CHANNELS_COUNT 3

/*! @brief Ticks per second. */
#define HAL_GPT_TICKS_PER_SECOND 6000000

typedef struct {
    GPT_Type *base;
    clock_root_control_t root_clk;
    IRQn_Type irqn;
} _HalGptDevice;

typedef struct {
    gpt_output_compare_channel_t number;
    gpt_interrupt_enable_t intr_mask;
    gpt_status_flag_t flag;
} _HalGptChannel;

typedef struct {
    size_t index;
    const _HalGptDevice *device;
    const _HalGptChannel *channel;
    void (*callbacks)(void *);
    void *user_data;
} HalGpt;
