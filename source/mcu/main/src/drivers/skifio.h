/*!
 * @brief Driver for Tornado DAC/ADC board.
 */

#pragma once

#include <hal/defs.h>

#define SKIFIO_AI_CHANNEL_COUNT 6

#define SKIFIO_DI_SIZE 8
#define SKIFIO_DO_SIZE 4

typedef int32_t SkifioAi;
typedef int32_t SkifioAo;

typedef struct SkifioInput {
    SkifioAi ais[SKIFIO_AI_CHANNEL_COUNT];
    int8_t temp;
    uint8_t status;
} SkifioInput;

typedef struct SkifioOutput {
    SkifioAo ao;
} SkifioOutput;

typedef uint8_t SkifioDi;
typedef uint8_t SkifioDo;
typedef void (*SkifioDiCallback)(void *, SkifioDi);

hal_retcode skifio_init();
hal_retcode skifio_deinit();

hal_retcode skifio_ao_enable();
hal_retcode skifio_ao_disable();

hal_retcode skifio_transfer(const SkifioOutput *out, SkifioInput *in);
hal_retcode skifio_wait_ready(uint32_t delay_ms);

hal_retcode skifio_do_write(SkifioDo value);

SkifioDi skifio_di_read();
hal_retcode skifio_di_subscribe(SkifioDiCallback callback, void *data);
hal_retcode skifio_di_unsubscribe();
