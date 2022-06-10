#pragma once

#include <ipp.h>


#define ADC_COUNT 6

#define SAMPLE_FREQ_HZ 10000

#define DAC_MAX_ABS_V 10.0
#define ADC_MAX_ABS_V 10.0

#define DAC_CODE_SHIFT 32767
#define DAC_STEP_UV 315.7445
#define ADC_STEP_UV 346.8012

typedef int32_t point_t;

#define RPMSG_MAX_APP_MSG_LEN 496
#define RPMSG_MAX_MCU_MSG_LEN 496

#define _dac_msg_max_points_by_len(len) \
    (((len) - sizeof(((IppAppMsg *)NULL)->type) - sizeof(IppAppMsgDacData)) / sizeof(point_t))
#define _adc_msg_max_points_by_len(len) \
    (((len) - sizeof(((IppMcuMsg *)NULL)->type) - sizeof(IppMcuMsgAdcData)) / (ADC_COUNT * sizeof(point_t)))

#define DAC_MSG_MAX_POINTS _dac_msg_max_points_by_len(RPMSG_MAX_APP_MSG_LEN)
#define ADC_MSG_MAX_POINTS _adc_msg_max_points_by_len(RPMSG_MAX_MCU_MSG_LEN)


#define KEEP_ALIVE_PERIOD_MS 100
#define KEEP_ALIVE_MAX_DELAY_MS 200
