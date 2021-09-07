#include <hal/io.h>

#include <hal/assert.h>

#ifdef HAL_PRINT_RPMSG

static bool io_rpmsg_initialized = false;
static hal_rpmsg_channel io_rpmsg_channel; 

void hal_io_rpmsg_init(hal_rpmsg_channel *channel) {
    if (channel != NULL) {
        io_rpmsg_channel = *channel;
        io_rpmsg_initialized = true;
    } else {
        io_rpmsg_initialized = false;
    }
}

hal_rpmsg_channel *__hal_io_rpmsg_channel() {
    if (io_rpmsg_initialized) {
        return &io_rpmsg_channel;
    } else {
        return NULL;
    }
}

uint8_t *__hal_io_rpmsg_alloc_buffer(size_t *size) {
    hal_assert(io_rpmsg_initialized);
    uint8_t *buffer;
    hal_assert(HAL_SUCCESS == hal_rpmsg_alloc_tx_buffer(&io_rpmsg_channel, &buffer, size, HAL_WAIT_FOREVER));
    return buffer;
}

void __hal_io_rpmsg_send_debug_message(uint8_t unused, uint8_t *buffer) {
    hal_assert(io_rpmsg_initialized);
    IppMcuMsg *msg = (IppMcuMsg *)buffer;
    msg->type = IPP_MCU_MSG_DEBUG;
    hal_assert(HAL_SUCCESS == hal_rpmsg_send_nocopy(&io_rpmsg_channel, buffer, ipp_mcu_msg_size(msg)));
}

void __hal_io_rpmsg_send_error_message(uint8_t code, uint8_t *buffer) {
    hal_assert(io_rpmsg_initialized);
    IppMcuMsg *msg = (IppMcuMsg *)buffer;
    msg->type = IPP_MCU_MSG_ERROR;
    msg->error.code = code;
    hal_assert(HAL_SUCCESS == hal_rpmsg_send_nocopy(&io_rpmsg_channel, buffer, ipp_mcu_msg_size(msg)));
}

#endif
