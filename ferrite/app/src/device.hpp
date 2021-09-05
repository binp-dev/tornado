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
#include <core/mpsc.hpp>
#include <channel/base.hpp>
#include <encoder.hpp>
#include <ipp.hpp>


class Device {
private:
    struct AdcValue {
        uint8_t channel_no;
        uint32_t value;
    };

    // Shared data
    std::atomic_bool done;

    // Device support data
    std::mutex mutex; // FIXME: Allow concurrent operation.
    std::thread worker;
    std::optional<mpsc::Receiver<AdcValue>> adc_out;

    // Worker data
    std::unique_ptr<Channel> channel;
    std::optional<mpsc::Sender<AdcValue>> adc_in;

private:
    void serve_loop() {
        std::cout << "[ioc] Channel serve thread started" << std::endl;
        const auto timeout = std::chrono::milliseconds(10);

        channel->send(ipp::MsgAppAny{ipp::MsgAppStart{}}, std::nullopt).unwrap(); // Wait forever
        std::cout << "[ioc] Handshake sent" << std::endl;

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

            // TODO: Use visit with overloaded lambda
            if(std::holds_alternative<ipp::MsgMcuAdcVal>(incoming.variant())) {
                const auto adc_val = std::get<ipp::MsgMcuAdcVal>(incoming.variant());
                adc_in->send(AdcValue{adc_val.index(), adc_val.value()});

            } else if (std::holds_alternative<ipp::MsgMcuDebug>(incoming.variant())) {
                std::cout << "Device: " << std::get<ipp::MsgMcuDebug>(incoming.variant()).message() << std::endl;

            } else if (std::holds_alternative<ipp::MsgMcuError>(incoming.variant())) {
                const auto &inc_err = std::get<ipp::MsgMcuError>(incoming.variant());
                std::cout << "Device Error (0x" << std::hex << int(inc_err.code()) << std::dec << "): " << inc_err.message() << std::endl;

            } else {
                unimplemented();
            }
        }
    }

    // Device support methods
public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;

    Device(std::unique_ptr<Channel> channel) : channel(std::move(channel)) {
        auto [in, out] = mpsc::make_channel<AdcValue>();
        adc_in = std::move(in);
        adc_out = std::move(out);

        std::lock_guard<std::mutex> device_guard(mutex);

        done.store(false);
        worker = std::thread([this]() { this->serve_loop(); });
    }
    virtual ~Device() {
        done.store(true);
        worker.join();
    }

    void write_dac(uint32_t value) {
        std::lock_guard<std::mutex> device_guard(mutex);

        ipp::MsgAppDacSet outgoing{value};
        channel->send(ipp::MsgAppAny{std::move(outgoing)}, std::nullopt).unwrap();
    }

    uint32_t read_adc(uint8_t index) {
        std::lock_guard<std::mutex> device_guard(mutex);

        assert_false(adc_out->try_receive().has_value());

        ipp::MsgAppAdcReq outgoing{index};
        channel->send(ipp::MsgAppAny{std::move(outgoing)}, std::nullopt).unwrap();

        const auto adc_value = adc_out->receive();
        assert_true(adc_value.has_value());
        assert_eq(adc_value->channel_no, index);
        return adc_value->value;
    }
};
