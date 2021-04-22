#pragma once

#include <rpmsg/rpmsg_rtos.h>

struct hal_rpmsg_channel {
    struct remote_device *rdev;
    struct rpmsg_channel *app_chnl;
};
