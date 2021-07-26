#pragma once

#if !defined(HAL_IMX7)
#error "This header should be included only when building for i.MX7"
#endif

#include <rpmsg/rpmsg_rtos.h>

struct hal_rpmsg_channel {
    struct remote_device *rdev;
    struct rpmsg_channel *app_chnl;
};
