/*!
 * @brief Драйвер для платы ЦАП/АЦП Tornado.
 */

#pragma once

#include <hal/defs.h>

#define _SKIFIO_DEBUG

#define SKIFIO_ADC_CHANNEL_COUNT 6

typedef struct SkifioInput {
    int32_t adcs[SKIFIO_ADC_CHANNEL_COUNT];
} SkifioInput;

typedef struct SkifioOutput {
    int16_t dac;
} SkifioOutput;

hal_retcode skifio_init();
hal_retcode skifio_deinit();

hal_retcode skifio_transfer(const SkifioOutput *out, SkifioInput *in);
hal_retcode skifio_wait_ready(uint32_t delay_ms);

#ifdef _SKIFIO_DEBUG
typedef struct {
    volatile uint64_t intr_count;
} _SkifioDebugInfo;

extern _SkifioDebugInfo _SKIFIO_DEBUG_INFO;
#endif
