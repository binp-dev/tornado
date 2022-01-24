/*!
 * @file app_gpt.h
 * @brief Application-specific GPT (general purpose timer) abstraction layer
 */

#pragma once

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#define APP_GPT_SEC 6000000 /*! GPT clock count equal to one second */


/*! @brief Initialize GPT hardware, usually called from `hardware_init()`. */
void APP_GPT_HardwareInit();

/*!
 * @brief Configure GPT subsystem.
 *
 * @param period GPT clock count.
 * @param target Semaphore to be given on timer event.
 * @return Status, zero on success.
 */
uint8_t APP_GPT_Init(uint32_t period, SemaphoreHandle_t target);
