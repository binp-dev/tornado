#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>

#include <channel/channel.hpp>

class Device {
private:
    std::vector<double> waveforms[3];
    std::atomic_bool swap_ready;
    std::mutex swap_mutex;

    std::unique_ptr<Channel> channel;

    std::atomic_bool done;
    std::thread worker;

public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;

    Device(
        std::unique_ptr<Channel> channel,
        size_t waveform_point_count
    ) :
        swap_ready(false),
        channel(std::move(channel))
    {
        for (int i = 0; i < 3; ++i) {
            waveforms[i].resize(waveform_point_count, 0.0);
        }
    }
    virtual ~Device() {

    }

    void set_waveform(double *points, size_t count) {

    }
};
