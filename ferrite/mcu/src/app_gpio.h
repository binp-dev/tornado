#pragma once

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#define APP_GPIO_MODE_INPUT  1
#define APP_GPIO_MODE_OUTPUT 2


void APP_GPIO_HardwareInit();
uint8_t APP_GPIO_Init(uint32_t mode, SemaphoreHandle_t sem);
uint8_t APP_GPIO_Set(uint8_t on);
