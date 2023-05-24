#pragma once

#include <rpmsg_lite.h>
#include <rpmsg_queue.h>
#include <rpmsg_ns.h>

struct hal_rpmsg_channel {
    rpmsg_queue_handle queue;
    struct rpmsg_lite_endpoint *ept;
    uint32_t remote_addr;
};
