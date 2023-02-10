#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "defs.h"

#if defined(HAL_IMX7)
#include "imx7/gpt.h"
#elif defined(HAL_IMX8MN)
#include "imx8mn/gpt.h"
#else
#error "Unknown target"
#endif

/*!
 * @brief Initialize GPT hardware.
 * @param[out] gpt GPT handle to be written on success.
 * @param[in] instance GPT instance index.
 * @return Return code.
 */
hal_retcode hal_gpt_init(HalGpt *gpt, uint32_t instance);

/*!
 * @brief Deinitialize GPT hardware.
 * @param[in] gpt GPT handle.
 * @return Return code.
 */
hal_retcode hal_gpt_deinit(HalGpt *gpt);

/*!
 * @brief Start GPT.
 * @param[in] gpt GPT handle.
 * @param[in] channel Compare channel number. FIXME: Currently only channel = 1 is supported.
 * @param[in] period_us Period in microseconds.
 * @param[in] callback Called from interrupt on compare event.
 * @param[in] user_data Data passed to callback.
 * @return Return code.
 */
hal_retcode hal_gpt_start(
    HalGpt *gpt,
    uint32_t channel,
    uint32_t period_us,
    void (*callback)(void *),
    void *user_data //
);

/*!
 * @brief Stop GPT.
 * @param[in] instance GPT handle.
 * @return Return code.
 */
hal_retcode hal_gpt_stop(HalGpt *gpt);
