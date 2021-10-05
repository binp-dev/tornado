/*!
 * @brief Драйвер для платы ЦАП/АЦП Tornado.
 */

#pragma once

#include <hal/defs.h>

#define SKIFIO_ADC_CHANNEL_COUNT 6

typedef struct SkifioInput {
    uint32_t adc[SKIFIO_ADC_CHANNEL_COUNT];
} SkifioInput;

typedef struct SkifioOutput {
    uint16_t dac;
} SkifioOutput;

hal_retcode skifio_init();
hal_retcode skifio_deinit();

hal_retcode skifio_transfer(const SkifioOutput *out, SkifioInput *in);
