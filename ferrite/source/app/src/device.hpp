#pragma once

#include <array>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
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
constexpr auto START_DELAY = std::chrono::seconds(2); // FIXME: Signal from outside
constexpr auto ADC_REQ_PERIOD = std::chrono::seconds(1);

class Device {
private:
    struct AdcEntry {
        std::atomic<int32_t> value;
        std::function<void()> notify;
    };

    struct DacEntry {
        std::atomic<int32_t> value;
        std::atomic<bool> update = false;
    };

    struct DinEntry {
        std::atomic<uint8_t> value;
        std::function<void()> notify;
    };

    struct DoutEntry {
        std::atomic<uint8_t> value;
        std::atomic<bool> update = false;
    };

private:
    std::atomic_bool done;
    std::thread recv_worker;
    std::thread send_worker;
    std::condition_variable send_ready;
    std::mutex send_mutex;

    std::array<AdcEntry, ADC_COUNT> adcs;
    DacEntry dac;
    DinEntry din;
    DoutEntry dout;

    std::unique_ptr<Channel> channel;

private:
    void recv_loop() {
        std::this_thread::sleep_for(START_DELAY);

        std::cout << "[app] Channel serve thread started" << std::endl;
        const auto timeout = std::chrono::milliseconds(10);

        channel->send(ipp::AppMsg{ipp::AppMsgStart{}}, std::nullopt).unwrap(); // Wait forever
        std::cout << "[app] Start signal sent" << std::endl;

        send_worker = std::thread([this]() { this->send_loop(); });

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
                    auto &adc = adcs[i];
                    adc.value.store(adc_val.values[i]);
                    if (adc.notify) {
                        adc.notify();
                    }
                }

            } else if (std::holds_alternative<ipp::McuMsgDinVal>(incoming.variant)) {
                const auto din_val = std::get<ipp::McuMsgDinVal>(incoming.variant);
                std::cout << "Din updated: " << uint32_t(din_val.value) << std::endl;
                din.value.store(din_val.value);
                if (din.notify) {
                    din.notify();
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

        send_ready.notify_all();
        send_worker.join();
    }

    void send_loop() {
        std::cout << "[app] ADC req thread started" << std::endl;

        auto next_wakeup = std::chrono::system_clock::now();
        while (!this->done.load()) {
            std::unique_lock send_lock(send_mutex);
            auto status = send_ready.wait_until(send_lock, next_wakeup);

            if (status == std::cv_status::timeout) {
                // std::cout << "[app] Request ADC measurements." << std::endl;
                channel->send(ipp::AppMsg{ipp::AppMsgAdcReq{}}, std::nullopt).unwrap();
                next_wakeup = std::chrono::system_clock::now() + ADC_REQ_PERIOD;
            }
            if (dac.update.exchange(false)) {
                int32_t value = dac.value.load();
                std::cout << "[app] Send DAC value: " << value << std::endl;
                channel->send(ipp::AppMsg{ipp::AppMsgDacSet{value}}, std::nullopt).unwrap();
            }
            if (dout.update.exchange(false)) {
                uint8_t value = dout.value.load();
                std::cout << "[app] Send Dout value: " << value << std::endl;
                channel->send(ipp::AppMsg{ipp::AppMsgDoutSet{uint8_t(value)}}, std::nullopt).unwrap();
            }
        }
    }

public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;

    Device(std::unique_ptr<Channel> channel) : channel(std::move(channel)) {
        done.store(false);
        recv_worker = std::thread([this]() { this->recv_loop(); });
    }
    virtual ~Device() {
        done.store(true);
        recv_worker.join();
    }

public:
    void write_dac(int32_t value) {
        {
            std::lock_guard send_guard(send_mutex);
            dac.value.store(value);
            dac.update.store(true);
        }
        send_ready.notify_all();
    }

    int32_t read_adc(size_t index) {
        assert_true(index < ADC_COUNT);
        return adcs[index].value.load();
    }
    void set_adc_callback(size_t index, std::function<void()> &&callback) {
        assert_true(index < ADC_COUNT);
        adcs[index].notify = std::move(callback);
    }

    void write_dout(uint32_t value) {
        {
            std::lock_guard send_guard(send_mutex);
            dac.value.store(value);
            dac.update.store(true);
        }
        send_ready.notify_all();
    }

    uint32_t read_din() {
        return din.value.load();
    }
    void set_din_callback(std::function<void()> &&callback) {
        din.notify = std::move(callback);
    }
};
