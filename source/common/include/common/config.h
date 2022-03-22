#pragma once

#include <ipp.h>


#define ADC_COUNT 6

#define DAC_SHIFT 32767
#define DAC_STEP_UV 315.7445
#define ADC_STEP_UV 346.8012

typedef int32_t point_t;

#define RPMSG_MAX_MSG_LEN 256 // 512

#define DAC_MSG_MAX_POINTS \
    ((RPMSG_MAX_MSG_LEN - sizeof(((IppAppMsg *)NULL)->type) - sizeof(IppAppMsgDacData)) / sizeof(point_t))
#define ADC_MSG_MAX_POINTS \
    ((RPMSG_MAX_MSG_LEN - sizeof(((IppMcuMsg *)NULL)->type) - sizeof(IppMcuMsgAdcData)) / sizeof(point_t))


#define KEEP_ALIVE_PERIOD_MS 100
#define KEEP_ALIVE_MAX_DELAY_MS 200
