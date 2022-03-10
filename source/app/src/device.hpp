#pragma once

#include <array>
#include <deque>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <functional>

#include <core/mutex.hpp>
#include <core/collections/vec_deque.hpp>

#include <common/config.h>
#include <ipp.hpp>
#include <channel/message.hpp>

#include "double_buffer.hpp"

using DeviceChannel = MessageChannel<ipp::AppMsg, ipp::McuMsg>;

class Device final {
public:
    enum class DacOperationState {
        Stopped = 0,
        Running,
    };

    enum class DacPlaybackMode {
        OneShot = 0,
        Cyclic,
    };

private:
    struct DinEntry {
        std::atomic<uint8_t> value;
        std::function<void()> notify;
    };

    struct DoutEntry {
        std::atomic<uint8_t> value;
        std::atomic<bool> update{false};
    };

    struct AdcEntry {
        Mutex<VecDeque<int32_t>> data;
        size_t max_size;
        std::function<void()> notify;
        std::atomic<int32_t> last_value{0};
    };

    struct DacEntry {
        DoubleBuffer<int32_t> data;
        Vec<int32_t> tmp_buf;

        std::atomic<bool> has_mcu_req{false};

        std::function<void()> sync_ioc_request_flag;
        std::atomic<bool> ioc_requested{false};
    };

private:
    std::atomic_bool done;
    std::thread recv_worker;
    std::thread send_worker;
    std::condition_variable send_ready;
    std::mutex send_mutex;

    const size_t msg_max_len_;
    const std::chrono::milliseconds keep_alive_period_{KEEP_ALIVE_PERIOD_MS};

    DinEntry din;
    DoutEntry dout;
    std::array<AdcEntry, ADC_COUNT> adcs;
    DacEntry dac;

    DeviceChannel channel;

private:
    void recv_loop();
    void send_loop();

public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;
    Device(Device &&dev) = delete;
    Device &operator=(Device &&dev) = delete;

    Device(std::unique_ptr<Channel> &&channel, size_t msg_max_len);
    ~Device();

    void start();
    void stop();

public:
    void write_dout(uint32_t value);

    uint32_t read_din();
    void set_din_callback(std::function<void()> &&callback);

    void init_dac(size_t max_len);
    void write_dac(const int32_t *data, size_t len);

    void init_adc(uint8_t index, size_t max_size);
    void set_adc_callback(size_t index, std::function<void()> &&callback);
    std::vector<int32_t> read_adc(size_t index);
    int32_t read_adc_last_value(size_t index);

    [[nodiscard]] bool dac_req_flag();
    void set_dac_req_callback(std::function<void()> &&callback);

    void set_dac_playback_mode(DacPlaybackMode mode);
    void set_dac_operation_state(DacOperationState state);
};
