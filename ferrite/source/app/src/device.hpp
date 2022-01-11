#pragma once

#include <array>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstring>
#include <cassert>
#include <variant>

#include <core/assert.hpp>
#include <core/panic.hpp>
#include <core/mpsc.hpp>
#include <channel/base.hpp>
#include <encoder.hpp>
#include <ipp.hpp>

constexpr size_t ADC_COUNT = 6;

class Device {
private:
    struct AdcEntry {
        std::atomic<int32_t> value;
        std::function<void()> callback;
    };

    struct DacEntry {
        std::atomic<int32_t> value;
        std::atomic<bool> update = false;
    };

    // Shared data
    std::atomic_bool done;
    std::mutex device_mutex; // FIXME: Allow concurrent operation.
    std::array<AdcEntry, ADC_COUNT> adcs;
    DacEntry dac;

    // Device support data
    std::thread recv_worker;
    std::thread adc_req_worker;

    // Communication channel
    std::unique_ptr<Channel> channel;
    std::mutex channel_mutex;

    // ADC request period
    std::chrono::milliseconds adc_req_period;

private:
    void recv_loop() {
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

                //static_assert(decltype(adcs)::size() == decltype(adc_val.values)::size());
                for (size_t i = 0; i < ADC_COUNT; ++i) {
                    adcs[i].value.store(adc_val.values[i]);

                    if (adcs[i].callback) {
                        adcs[i].callback();
                    }
                }

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

    void adc_req_loop() {
        for(size_t i = 0; !this->done.load(); ++i) {
            std::this_thread::sleep_for(adc_req_period);
            {
                std::lock_guard channel_guard(channel_mutex);

                if (dac.update.exchange(false)) {
                    int32_t value = dac.value.load();
                    std::cout << "[app] Send DAC value: " << value << std::endl;
                    channel->send(ipp::AppMsg{ipp::AppMsgDacSet{value}}, std::nullopt).unwrap();
                }

                std::cout << "[app] Request ADC measurements: " << i << std::endl;
                channel->send(ipp::AppMsg{ipp::AppMsgAdcReq{}}, std::nullopt).unwrap();
            }
        }
    }

public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;

    Device(std::unique_ptr<Channel> channel, std::chrono::milliseconds period) :
        channel(std::move(channel)),
        adc_req_period(period)
    {
        std::lock_guard device_guard(device_mutex);

        done.store(false);
        recv_worker = std::thread([this]() { this->recv_loop(); });
        adc_req_worker = std::thread([this]() { this->adc_req_loop(); });
    }
    virtual ~Device() {
        done.store(true);
        adc_req_worker.join();
        recv_worker.join();
    }

    void write_dac(int32_t value) {
        std::lock_guard device_guard(device_mutex);
        dac.value.store(value);
        dac.update.store(true);
    }

    int32_t read_adc(size_t index) {
        std::lock_guard device_guard(device_mutex);
        
        assert_true(index < ADC_COUNT);
        return adcs[index].value.load();
    }

    void set_adc_callback(size_t index, std::function<void()> && callback) {
        std::lock_guard device_guard(device_mutex);
        
        assert_true(index < ADC_COUNT);
        adcs[index].callback = std::move(callback);
    }
};
