#include <external.hpp>

#include <channel/rpmsg.hpp>

std::unique_ptr<Channel> make_device_channel() {
    return std::make_unique<RpmsgChannel>(std::move(RpmsgChannel::create("/dev/ttyRPMSG0").unwrap()));
}
