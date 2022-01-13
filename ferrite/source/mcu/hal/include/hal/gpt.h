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
 * @param[in] instance GPT instance index.
 * @return Return code.
 */
hal_retcode hal_gpt_init(uint32_t instance);

/*!
 * @brief Deinitialize GPT hardware.
 * @param[in] instance GPT instance index.
 * @return Return code.
 */
hal_retcode hal_gpt_deinit(uint32_t instance);

/*!
 * @brief Start GPT.
 * @param[in] instance GPT instance index.
 * @param[in] period GPT ticks count.
 * @param[in] target Semaphore to be given on timer event. FIXME: Replace with callback.
 * @return Return code.
 */
hal_retcode hal_gpt_start(uint32_t instance, uint32_t period, SemaphoreHandle_t target);

/*!
 * @brief Stop GPT.
 * @param[in] instance GPT instance index.
 * @return Return code.
 */
hal_retcode hal_gpt_stop(uint32_t instance);
