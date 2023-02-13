#pragma once

#include <c/config.h>
#include <ipp.h>

typedef Point point_t;

#define RPMSG_MAX_APP_MSG_LEN MAX_APP_MSG_LEN
#define RPMSG_MAX_MCU_MSG_LEN MAX_MCU_MSG_LEN

#define _dac_msg_max_points_by_len(len) \
    (((len) - sizeof(((IppAppMsg *)NULL)->type) - sizeof(IppAppMsgDacData)) / sizeof(point_t))
#define _adc_msg_max_points_by_len(len) \
    (((len) - sizeof(((IppMcuMsg *)NULL)->type) - sizeof(IppMcuMsgAdcData)) / (ADC_COUNT * sizeof(point_t)))

#define DAC_MSG_MAX_POINTS _dac_msg_max_points_by_len(RPMSG_MAX_APP_MSG_LEN)
#define ADC_MSG_MAX_POINTS _adc_msg_max_points_by_len(RPMSG_MAX_MCU_MSG_LEN)
