#pragma once

#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <hal/rpmsg.h>
#include <ipp.h>

#include <common/config.h>
#include <tasks/control.h>
#include <tasks/stats.h>


#define DAC_MSG_MAX_POINTS \
    ((RPMSG_MAX_MSG_LEN - sizeof(((IppAppMsg *)NULL)->type) - sizeof(IppAppMsgDacData)) / sizeof(point_t))
#define ADC_MSG_MAX_POINTS \
    ((RPMSG_MAX_MSG_LEN - sizeof(((IppMcuMsg *)NULL)->type) - sizeof(IppMcuMsgAdcData)) / sizeof(point_t))

typedef struct {
    hal_rpmsg_channel channel;
    bool alive;

    SemaphoreHandle_t send_sem;
    size_t dac_requested;

    ControlSync control_sync;
    Control *control;
    Statistics *stats;
} Rpmsg;


void rpmsg_init(Rpmsg *rpmsg, Control *control, Statistics *stats);
void rpmsg_deinit(Rpmsg *rpmsg);

/// Start rpmsg tasks.
void rpmsg_run(Rpmsg *rpmsg);
