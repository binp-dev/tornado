#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstring>
#include <cassert>

#include <common/proto.h>
#include <utils/slice.hpp>
#include <utils/panic.hpp>
#include <channel/channel.hpp>
#include <encoder.hpp>


#define TIMEOUT 10 // ms


class Device {
private:
    const size_t _max_points;
    const size_t _max_transfer;

    // Shared data
    std::vector<double> waveforms[3];
    std::atomic_bool swap_ready;
    std::mutex swap_mutex;
    std::atomic_bool done;

    // Worker data
    std::unique_ptr<Channel> channel;
    std::unique_ptr<const ArrayEncoder<double>> encoder;
    std::vector<uint8_t> send_buffer, recv_buffer;
    size_t waveform_pos = 0;

    // Device support data
    std::thread worker;

    // Worker methods
private:
    void fill(uint8_t **dst, size_t *dst_size) {
        std::pair<size_t, size_t> shift = encoder->encode(
            *dst, *dst_size,
            waveforms[0].data() + waveform_pos,
            waveforms[0].size() - waveform_pos
        );

        *dst += shift.first;
        *dst_size -= shift.first;
        waveform_pos += shift.second;
    }

    bool try_swap() {
        if (waveform_pos >= waveforms[0].size()) {
            if (swap_ready.load()) {
                std::lock_guard<std::mutex> guard(swap_mutex);
                std::swap(waveforms[0], waveforms[1]);
                swap_ready.store(false);
            }
            waveform_pos = 0;
            return true;
        } else {
            return false;
        }
    }

    size_t fill_until_full(uint8_t *data, size_t size) {
        size_t orig_size = size;

        try_swap();
        fill(&data, &size);

        while (true) {
            if (!try_swap()) {
                break;
            }
            fill(&data, &size);
        }

        return orig_size - size;
    }

    void serve_loop() {
        try {
            std::cout << "[ioc] Channel serve thread started" << std::endl;

            while(!this->done.load()) {
                size_t size = 0;
                try {
                    size = channel->receive(recv_buffer.data(), recv_buffer.size(), TIMEOUT);
                } catch (const Channel::TimedOut &e) {
                    continue;
                }

                uint8_t cmd = recv_buffer[0];
                std::cout << "M4 command: " << int(cmd) << std::endl;

                if(cmd == PSCM_WF_REQ) {
                    send_buffer[0] = PSCA_WF_DATA; 
                    size_t shift = 1;
                    size_t msg_size = fill_until_full(
                        send_buffer.data() + shift,
                        send_buffer.size() - shift
                    );
                    channel->send(send_buffer.data(), msg_size, TIMEOUT);
                } else {
                    throw Exception("Unknown PSCM command: " + std::to_string(cmd));
                }
            }
        } catch(const Exception &e) {
            std::cerr << "Exception in thread `serve_loop`:" << std::endl;
            std::cerr << e.what() << std::endl;
            panic();
        } catch(...) {
            std::cerr << "Unknown exception in thread `serve_loop`" << std::endl;
            panic();
        }
    }

public:
    // Device support methods
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;

    Device(
        std::unique_ptr<Channel> channel,
        std::unique_ptr<const ArrayEncoder<double>> encoder,
        size_t max_points,
        size_t max_transfer
    ) :
        _max_points(max_points),
        _max_transfer(max_transfer),

        swap_ready(false),

        channel(std::move(channel)),
        encoder(std::move(encoder)),

        send_buffer(max_transfer, 0),
        recv_buffer(max_transfer, 0)
    {
        for (int i = 0; i < 3; ++i) {
            waveforms[i].resize(max_points, 0.0);
        }

        done.store(false);
        worker = std::thread([this]() { this->serve_loop(); });
    }
    virtual ~Device() {
        done.store(true);
        worker.join();
    }

    void set_waveform(const_slice<double> waveform) {
        assert(waveform.size() <= max_points());
        if (!swap_ready.load()) {
            memcpy(waveforms[1].data(), waveform.data(), waveform.size());
            swap_ready.store(true);
        } else {
            memcpy(waveforms[2].data(), waveform.data(), waveform.size());
            {
                std::lock_guard<std::mutex> guard(swap_mutex);
                std::swap(waveforms[1], waveforms[2]);
                swap_ready.store(true);
            }
        }
    }

    size_t max_points() const {
        return _max_points;
    }
    size_t max_transfer() const {
        return _max_transfer;
    }
};
