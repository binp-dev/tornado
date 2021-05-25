/*!
 * @file gpt.h
 * @brief Application-specific GPT (general purpose timer) abstraction layer
 */

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "semphr.h"

/*! @brief Ticks per second. */
extern const uint32_t hal_gpt_ticks_per_second;

/*!
 * @brief Configure GPT hardware.
 * @param[in] instance GPT instance index.
 * @return Return code.
 */
uint8_t hal_gpt_init(uint32_t instance);

/*!
 * @brief Start GPT.
 * @param[in] instance GPT instance index.
 * @param[in] period GPT ticks count.
 * @param[in] target Semaphore to be given on timer event.
 * @return Return code.
 */
uint8_t hal_gpt_start(uint32_t instance, uint32_t period, SemaphoreHandle_t target);

/*!
 * @brief Stop GPT.
 * @param[in] instance GPT instance index.
 * @return Return code.
 */
uint8_t hal_gpt_stop(uint32_t instance);
