#pragma once

#include <memory>

#include <channel/base.hpp>

size_t max_message_length();
std::unique_ptr<Channel> make_device_channel();
