#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstring>
#include <cassert>

#include <core/panic.hpp>
#include <channel/base.hpp>
#include <encoder.hpp>
#include <../../common/proto.h>


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
        std::cout << "[ioc] Channel serve thread started" << std::endl;
        auto timeout = std::chrono::milliseconds(10);
        
        uint8_t start_sid = PSCA_START;
        channel->send(&start_sid, 1, std::nullopt).unwrap(); // Wait forever

        while(!this->done.load()) {
            size_t size = 0;
            auto result = channel->receive(recv_buffer.data(), recv_buffer.size(), timeout);
            if (result.is_ok()) {
                size = result.unwrap();
            } else {
                auto err = result.unwrap_err();
                if (err.kind == Channel::ErrorKind::TimedOut) {
                    continue;
                } else {
                    panic("IO Error: " + err.message);
                }
            }

            uint8_t cmd = recv_buffer[0];
            std::cout << "Received command: 0x" << std::hex << int(cmd) << std::dec << std::endl;

            if(cmd == PSCM_WF_REQ) {
                if (size != 1) {
                    panic("Bad size of PSCM_WF_REQ command");
                }
                send_buffer[0] = PSCA_WF_DATA; 
                size_t shift = 1;
                size_t msg_size = fill_until_full(
                    send_buffer.data() + shift,
                    send_buffer.size() - shift
                ) + shift;
                channel->send(send_buffer.data(), msg_size, timeout).unwrap();
            } else if (cmd == PSCM_MESSAGE) {
                uint8_t len = recv_buffer[1];
                std::cout << "Message(" << int(len) << "): " << recv_buffer.data() + 2 << std::endl;
            } else {
                panic("Unknown PSCM command: " + std::to_string(cmd));
            }
        }
    }

    // Device support methods
public:
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

    void set_waveform(const double *wf_data, const size_t wf_len) {
        assert(wf_len <= max_points());
        if (!swap_ready.load()) {
            std::copy(wf_data, wf_data + wf_len, waveforms[1].begin());
            swap_ready.store(true);
        } else {
            std::copy(wf_data, wf_data + wf_len, waveforms[2].begin());
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
