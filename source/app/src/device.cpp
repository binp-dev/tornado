#include "device.hpp"

#include <variant>
#include <cstring>

#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/convert.hpp>
#include <core/match.hpp>
#include <core/collections/vec.hpp>
#include <ipp.hpp>

using namespace core;

void Device::recv_loop() {
    core_log_info("Channel recv thread started");
    const auto timeout = std::chrono::milliseconds(100);

    channel_.send(ipp::AppMsg{ipp::AppMsgConnect{}}, std::nullopt).unwrap(); // Wait forever
    core_log_info("Connect signal sent");
    send_worker_ = std::thread([this]() {
        this->send_loop();
    });

    while (!this->done_.load()) {
        auto result = channel_.receive(timeout);
        if (result.is_err()) {
            auto err = result.unwrap_err();
            if (err.kind == io::ErrorKind::TimedOut) {
                continue;
            } else {
                core_panic("IO Error: {}", err);
            }
        }
        auto incoming = result.unwrap();
        std::visit(
            overloaded{
                [&](ipp::McuMsgDinUpdate &&din_msg) {
                    din_.value.store(din_msg.value);
                    if (din_.notify) {
                        din_.notify();
                    }
                },
                [&](ipp::McuMsgAdcData &&adc_msg) {
                    const auto &points_arrays = adc_msg.points_arrays;
                    for (size_t i = 0; i < ADC_COUNT; ++i) {
                        auto &adc = adcs_[i];
                        // Remember last value.
                        if (points_arrays.size() > 0) {
                            adc.last_value.store(points_arrays.back()[i]);
                        }

                        // Convert codes to voltage.
                        Vec<double> tmp = std::move(adc.tmp_buf);
                        std::transform(
                            points_arrays.begin(),
                            points_arrays.end(),
                            std::back_inserter(tmp),
                            [&](std::array<point_t, ADC_COUNT> codes) {
                                return adc_code_to_volt(codes[i]);
                            } //
                        );

                        // Write chunk to queue.
                        auto data_guard = adc.data.lock();
                        core_assert(data_guard->write_array_exact(tmp));
                        tmp.clear();
                        adc.tmp_buf = std::move(tmp);

                        // Notify.
                        if (data_guard->size() >= adc.max_size && !adc.ioc_notified.load()) {
                            core_assert(adc.notify);
                            adc.ioc_notified.store(true);
                            adc.notify();
                        }
                    }
                },
                [&](ipp::McuMsgDacRequest &&dac_req_msg) {
                    {
                        // Note: `send_mutex_` must be locked even if atomic is used. See `std::condition_variable`
                        // reference.
                        std::lock_guard send_guard(send_mutex_);
                        dac_.mcu_requested_count += dac_req_msg.count;
                    }
                    send_ready_.notify_one();
                },
                [&](ipp::McuMsgDebug &&debug) {
                    core_log_debug("[mcu:debug]: {}", debug.message);
                },
                [&](ipp::McuMsgError &&error) {
                    core_log_error("[mcu:error] (code {}): {}", uint32_t(error.code), error.message);
                },
                [&](auto &&) {
                    core_unimplemented();
                },
            },
            std::move(incoming.variant) //
        );
    }

    send_ready_.notify_all();
    send_worker_.join();
}

void Device::send_loop() {
    core_log_info("Channel send thread started");
    const auto timeout = keep_alive_period_;
    auto next_wakeup = std::chrono::steady_clock::now();
    while (!this->done_.load()) {
        std::unique_lock send_lock(send_mutex_);
        auto status = send_ready_.wait_until(send_lock, next_wakeup);
        if (status == std::cv_status::timeout) {
            channel_.send(ipp::AppMsg{ipp::AppMsgKeepAlive{}}, timeout).unwrap();
            next_wakeup = std::chrono::steady_clock::now() + keep_alive_period_;
            // Sometimes `std::condition_variable` returns `std::cv_status::timeout` even when notified.
            // So we don't discard wakeup by timeout.
            // continue;
        }

        if (dout_.update.exchange(false)) {
            uint8_t value = dout_.value.load();
            core_log_debug("Send Dout value: {}", value);
            channel_.send(ipp::AppMsg{ipp::AppMsgDoutUpdate{uint8_t(value)}}, timeout).unwrap();
        }
        if (dac_.mcu_requested_count.load() > 0) {
            for (;;) {
                Vec<double> tmp = std::move(dac_.tmp_buf);
                ipp::AppMsgDacData dac_msg;

                // Read next chunk from double buffer.
                size_t max_count = std::min(
                    _dac_msg_max_points_by_len(channel_.max_message_length()),
                    dac_.mcu_requested_count.load() //
                );

                size_t count = dac_.data.read_array_into(tmp, max_count);
                dac_.mcu_requested_count -= count;

                if (count > 0) {
                    // Convert voltage to codes.
                    std::transform(
                        tmp.begin(),
                        tmp.end(),
                        std::back_inserter(dac_msg.points),
                        [&](double volt) {
                            return dac_volt_to_code(volt);
                        } //
                    );
                    tmp.clear();

                    // Send.
                    core_assert(dac_msg.packed_size() <= channel_.max_message_length() - 1);
                    channel_.send(ipp::AppMsg{std::move(dac_msg)}, timeout).unwrap();
                    dac_.tmp_buf = std::move(tmp);
                } else {
                    break;
                }
            }

            if (dac_.data.write_ready() && !dac_.ioc_requested.load() && dac_.sync_ioc_request_flag) {
                dac_.sync_ioc_request_flag();
                dac_.ioc_requested.store(true);
            }
        }
        if (stats_reset_.exchange(false)) {
            channel_.send(ipp::AppMsg{ipp::AppMsgStatsReset{}}, timeout).unwrap();
        }
    }
}

Device::Device(std::unique_ptr<Channel> &&raw_channel, size_t max_msg_len) :
    channel_(std::move(raw_channel), max_msg_len) //
{
    done_.store(true);
}
Device::~Device() {
    stop();
}

void Device::start() {
    done_.store(false);
    recv_worker_ = std::thread([this]() {
        this->recv_loop();
    });
}

void Device::stop() {
    if (!done_.load()) {
        done_.store(true);
        recv_worker_.join();
    }
}

void Device::write_dout(uint32_t value) {
    {
        constexpr uint32_t mask = 0xfu;
        if ((value & ~mask) != 0) {
            core_log_warning("Ignoring extra bits in dout ({})", uint32_t(value));
        }
        {
            // Note: `send_mutex_` must be locked even if atomic is used. See `std::condition_variable` reference.
            std::lock_guard send_guard(send_mutex_);
            dout_.value.store(uint8_t(value & mask));
            dout_.update.store(true);
        }
    }
    send_ready_.notify_one();
}

uint32_t Device::read_din() {
    return din_.value.load();
}
void Device::set_din_callback(std::function<void()> &&callback) {
    din_.notify = std::move(callback);
}

void Device::init_dac(const size_t max_size) {
    dac_.data.reserve(max_size);
}

void Device::write_dac(std::span<const double> data) {
    core_assert(dac_.data.write_array_exact(data));
    send_ready_.notify_one();
    if (dac_.sync_ioc_request_flag) {
        dac_.ioc_requested.store(false);
        dac_.sync_ioc_request_flag();
    }
}

void Device::init_adc(uint8_t index, size_t max_size) {
    adcs_[index].max_size = max_size;
}

void Device::set_adc_callback(size_t index, std::function<void()> &&callback) {
    core_assert(index < ADC_COUNT);
    adcs_[index].notify = std::move(callback);
}

std::vector<double> Device::read_adc(size_t index) {
    auto &adc = adcs_[index];

    Vec<double> data;
    size_t skipped_count = 0;
    {
        auto adc_data_guard = adc.data.lock();
        while (adc_data_guard->size() >= 2 * adc.max_size) {
            adc_data_guard->skip_front(adc.max_size);
            skipped_count += 1;
        }

        data.reserve(adc.max_size);
        core_assert_eq(data.write_array_from(*adc_data_guard, adc.max_size), adc.max_size);
    }
    adc.ioc_notified.store(false);

    if (skipped_count) {
        core_log_warning("Skipped {} ADC{} waveforms", skipped_count, uint32_t(index));
    }

    return data;
}

int32_t Device::read_adc_last_value(size_t index) {
    core_assert(index < ADC_COUNT);
    return adcs_[index].last_value.load();
}

bool Device::dac_req_flag() {
    return dac_.data.write_ready();
}

void Device::set_dac_req_callback(std::function<void()> &&callback) {
    dac_.sync_ioc_request_flag = std::move(callback);
}

void Device::set_dac_playback_mode(DacPlaybackMode mode) {
    switch (mode) {
    case DacPlaybackMode::OneShot:
        core_log_info("One-shot DAC mode set");
        dac_.data.set_cyclic(false);
        break;
    case DacPlaybackMode::Cyclic:
        core_log_info("Cyclic DAC mode set");
        dac_.data.set_cyclic(true);
        break;
    default:
        core_unreachable();
    }
}

void Device::set_dac_operation_state(DacOperationState) {
    core_log_warning("DAC operation state changing is not supported yet");
}

void Device::reset_statistics() {
    {
        // Note: `send_mutex_` must be locked even if atomic is used. See `std::condition_variable` reference.
        std::lock_guard send_guard(send_mutex_);
        stats_reset_.store(true);
    }
    send_ready_.notify_all();
}

point_t Device::dac_volt_to_code(double volt) const {
    return DAC_CODE_SHIFT + point_t((volt * 1e6) / DAC_STEP_UV);
}

double Device::adc_code_to_volt(point_t code) const {
    return (double(code) / 256.0) * ADC_STEP_UV * 1e-6;
}
