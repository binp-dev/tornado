#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstring>
#include <cassert>

#include <core/assert.hpp>
#include <core/panic.hpp>
#include <channel/base.hpp>
#include <encoder.hpp>
#include <ipp.hpp>


class Device {
private:
    const size_t _max_points;
    const size_t _max_transfer;

    // Global mutex
    // FIXME: Allow concurrent operation.
    std::mutex device_mutex;

    // Shared data
    std::vector<double> waveforms[3];
    std::atomic_bool swap_ready;
    std::mutex swap_mutex;
    std::atomic_bool done;

    // Worker data
    std::unique_ptr<Channel> channel;
    std::unique_ptr<const ArrayEncoder<double>> encoder;
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
        const auto timeout = std::chrono::milliseconds(10);

        channel->send(ipp::MsgAppAny{ipp::MsgAppStart{}}, std::nullopt).unwrap(); // Wait forever

        while(!this->done.load()) {
            auto result = channel->receive(timeout);
            if (result.is_err()) {
                auto err = result.unwrap_err();
                if (err.kind == Channel::ErrorKind::TimedOut) {
                    continue;
                } else {
                    // TODO: Use fmt
                    std::stringstream text;
                    text << err;
                    panic("IO Error: " + text.str());
                }
            }
            auto incoming = result.unwrap();

            /*
            std::cout << "Received command: ";
            std::visit([](const auto &m) {
                using Msg = typename std::remove_reference_t<decltype(m)>;
                std::cout << typeid(Msg).name() << " (0x" << std::hex << Msg::TYPE << std::dec << ")";
            }, incoming.variant());
            std::cout << std::endl;
            */

            // TODO: Use visit with overloaded lambda
            if(std::holds_alternative<ipp::MsgMcuWfReq>(incoming.variant())) {
                ipp::MsgAppWfData outgoing;
                auto &buffer = outgoing.data();
                const size_t min_len = ipp::MsgAppAny{ipp::MsgAppWfData{}}.length();
                assert_true(min_len < _max_transfer);
                buffer.resize(_max_transfer - min_len, 0);
                buffer.resize(fill_until_full(buffer.data(), buffer.size()));
                channel->send(ipp::MsgAppAny{std::move(outgoing)}, timeout).unwrap();

            } else if (std::holds_alternative<ipp::MsgMcuDebug>(incoming.variant())) {
                std::cout << "Device: " << std::get<ipp::MsgMcuDebug>(incoming.variant()).message() << std::endl;

            } else if (std::holds_alternative<ipp::MsgMcuError>(incoming.variant())) {
                const auto &inc_err = std::get<ipp::MsgMcuError>(incoming.variant());
                std::cout << "Device Error (0x" << std::hex << int(inc_err.code()) << std::dec << "): " << inc_err.message() << std::endl;

            } else {
                panic("Unexpected command");
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
        encoder(std::move(encoder))
    {
        std::lock_guard<std::mutex> device_guard(device_mutex);

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
        std::lock_guard<std::mutex> device_guard(device_mutex);

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
