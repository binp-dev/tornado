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
        core::Mutex<core::VecDeque<double>> data;
        core::Vec<double> tmp_buf;
        std::atomic<point_t> last_value{0};

        size_t max_size;
        std::function<void()> notify;
        std::atomic<bool> ioc_notified{false};
    };

    struct DacEntry {
        DoubleBuffer<double> data;
        core::Vec<double> tmp_buf;

        std::atomic<size_t> mcu_requested_count{0};

        std::function<void()> sync_ioc_request_flag;
        std::atomic<bool> ioc_requested{false};
    };

private:
    std::atomic_bool done_;
    std::thread recv_worker_;
    std::thread send_worker_;
    std::condition_variable send_ready_;
    std::mutex send_mutex_;

    const std::chrono::milliseconds keep_alive_period_{KEEP_ALIVE_PERIOD_MS};

    DinEntry din_;
    DoutEntry dout_;
    std::array<AdcEntry, ADC_COUNT> adcs_;
    DacEntry dac_;
    std::atomic<bool> stats_reset_{false};

    DeviceChannel channel_;

private:
    void recv_loop();
    void send_loop();

public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;
    Device(Device &&dev) = delete;
    Device &operator=(Device &&dev) = delete;

    explicit Device(std::unique_ptr<Channel> &&channel, size_t max_msg_len);
    ~Device();

    void start();
    void stop();

public:
    void write_dout(uint32_t value);

    uint32_t read_din();
    void set_din_callback(std::function<void()> &&callback);

    void init_dac(size_t max_len);
    void write_dac(std::span<const double> data);

    void init_adc(uint8_t index, size_t max_size);
    void set_adc_callback(size_t index, std::function<void()> &&callback);
    std::vector<double> read_adc(size_t index);
    point_t read_adc_last_value(size_t index);

    [[nodiscard]] bool dac_req_flag();
    void set_dac_req_callback(std::function<void()> &&callback);

    void set_dac_playback_mode(DacPlaybackMode mode);
    void set_dac_operation_state(DacOperationState state);

    void reset_statistics();

private:
    point_t dac_volt_to_code(double volt) const;
    double adc_code_to_volt(point_t code) const;
};
