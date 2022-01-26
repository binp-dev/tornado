#pragma once

#include <array>
#include <deque>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

#include <channel/base.hpp>


class Device final {
public:
    static constexpr size_t ADC_COUNT = 6;
    static constexpr size_t DAC_WF_BUFF_COUNT = 3;

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

    struct AdcWfEntry {
        std::deque<int32_t> wf_data;
        size_t wf_max_size;
        std::mutex mutex;
        std::function<void()> notify;
    };

    struct DacWfEntry {
        std::array<std::vector<int32_t>, DAC_WF_BUFF_COUNT> wf_data;
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
    std::array<AdcWfEntry, ADC_COUNT> adc_wfs;
    DacWfEntry dac_wf;

    std::unique_ptr<Channel> channel;
    std::chrono::milliseconds adc_req_period = std::chrono::milliseconds(1);

private:
    void recv_loop();
    void send_loop();

public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;
    Device(Device &&dev) = delete;
    Device &operator=(Device &&dev) = delete;

    Device(std::unique_ptr<Channel> channel);
    ~Device();

    void start();
    void stop();

public:
    void write_dac(int32_t value);

    int32_t read_adc(size_t index);
    void set_adc_callback(size_t index, std::function<void()> &&callback);
    void set_adc_req_period(std::chrono::milliseconds period);

    void write_dout(uint32_t value);

    uint32_t read_din();
    void set_din_callback(std::function<void()> &&callback);

    void init_dac_wf(size_t wf_max_size);

    void init_adc_wf(uint8_t index, size_t wf_max_size);
    void set_adc_wf_callback(size_t index, std::function<void()> &&callback);
    const std::vector<int32_t> read_adc_wf(size_t index);
};
