#include <external.hpp>

#include <common/config.h>
#include <channel/rpmsg.hpp>

size_t max_message_length() {
    return RPMSG_MAX_APP_MSG_LEN;
}

std::unique_ptr<Channel> make_device_channel() {
    return std::make_unique<RpmsgChannel>(std::move(RpmsgChannel::create("/dev/ttyRPMSG0").unwrap()));
}
