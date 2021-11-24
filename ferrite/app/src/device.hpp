#pragma once

#include <array>
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

constexpr size_t ADC_COUNT = 7; 

class Device {
private:
    struct AdcEntry {
        int32_t value = 0;
        std::function<void()> callback;
    };

    // Shared data
    std::atomic_bool done;
    std::mutex mutex; // FIXME: Allow concurrent operation.
    std::array<AdcEntry, ADC_COUNT> adcs;

    // Device support data
    std::thread worker;

    // Worker data
    std::unique_ptr<Channel> channel;

private:
    void serve_loop() {
        std::cout << "[app] Channel serve thread started" << std::endl;
        const auto timeout = std::chrono::milliseconds(10);

        channel->send(ipp::AppMsg{ipp::AppMsgStart{}}, std::nullopt).unwrap(); // Wait forever
        std::cout << "[app] Start signal sent" << std::endl;

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
            if(std::holds_alternative<ipp::McuMsgAdcVal>(incoming.variant)) {
                const auto adc_val = std::get<ipp::McuMsgAdcVal>(incoming.variant);
                adc_in->send(AdcValue{adc_val.index, adc_val.value});

            } else if (std::holds_alternative<ipp::McuMsgDebug>(incoming.variant)) {
                std::cout << "Device: " << std::get<ipp::McuMsgDebug>(incoming.variant).message << std::endl;

            } else if (std::holds_alternative<ipp::McuMsgError>(incoming.variant)) {
                const auto &inc_err = std::get<ipp::McuMsgError>(incoming.variant);
                std::cout << "Device Error (0x" << std::hex << int(inc_err.code) << std::dec << "): " << inc_err.message << std::endl;

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

        ipp::AppMsgDacSet outgoing{value};
        channel->send(ipp::AppMsg{std::move(outgoing)}, std::nullopt).unwrap();
    }

    uint32_t read_adc(uint8_t index) {
        std::lock_guard<std::mutex> device_guard(mutex);
        
        assert_true(index < ADC_COUNT);
        return adcs[index].value;
    }

    void set_adc_callback(uint8_t index, std::function<void()> && callback) {
        std::lock_guard<std::mutex> device_guard(mutex);
        
        assert_true(index < ADC_COUNT);
        adcs[index].callback = std::move(callback);
    }
};
