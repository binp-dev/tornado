#include <hal/rpmsg.h>

#include <stdlib.h>

size_t __hal_rpmsg_channel_size = sizeof(hal_rpmsg_channel);
size_t __hal_rpmsg_channel_align = __alignof__(hal_rpmsg_channel);
