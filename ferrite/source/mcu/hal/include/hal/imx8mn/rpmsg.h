#pragma once

#if !defined(HAL_IMX8MN)
#error "This header should be included only when building for i.MX8M Nano"
#endif

#include "rpmsg_lite.h"
#include "rpmsg_queue.h"
#include "rpmsg_ns.h"

struct hal_rpmsg_channel {
    rpmsg_queue_handle queue;
    struct rpmsg_lite_endpoint *ept;
    uint32_t remote_addr;
};
