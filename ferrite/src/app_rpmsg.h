/*!
 * @file app_flexcan.h
 * @brief Application-specific FLEXCAN abstraction layer
 */

#pragma once

#include <stdint.h>

void APP_RPMSG_HardwareInit();
uint8_t APP_RPMSG_Init();
uint8_t APP_RPMSG_Deinit();

int32_t APP_RPMSG_Send(const uint8_t *data, uint32_t len);
int32_t APP_RPMSG_Receive(uint8_t *data, uint32_t *len, uint32_t maxlen, uint32_t timeout);
