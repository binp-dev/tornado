#pragma once

#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <hal/rpmsg.h>

#include <common/config.h>
#include <tasks/control.h>
#include <tasks/stats.h>


#define DAC_MSG_MAX_POINTS ((RPMSG_MAX_MSG_LEN - sizeof(((IppAppMsg *)NULL)->type) - sizeof(IppAppMsgDacWf)) / sizeof(point_t))
#define ADC_MSG_MAX_POINTS ((RPMSG_MAX_MSG_LEN - sizeof(((IppMcuMsg *)NULL)->type) - sizeof(IppMcuMsgAdcWf)) / sizeof(point_t))
#define DAC_REQUEST_STEP DAC_MSG_MAX_POINTS

typedef struct {
    hal_rpmsg_channel channel;
    SemaphoreHandle_t send_sem;
    volatile bool alive;

    Control *control;
    Statistics *stats;
} Rpmsg;


void rpmsg_init(Rpmsg *rpmsg, Control *control, Statistics *stats);
void rpmsg_deinit(Rpmsg *rpmsg);

/// Start rpmsg tasks.
void rpmsg_run(Rpmsg *rpmsg);
