#pragma once

#include <memory>

#include <channel/base.hpp>

std::unique_ptr<Channel> make_device_channel();
