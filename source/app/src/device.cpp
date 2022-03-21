#include "device.hpp"

#include <variant>
#include <cstring>

#include <core/assert.hpp>
#include <core/panic.hpp>
#include <core/convert.hpp>
#include <core/match.hpp>
#include <core/collections/vec.hpp>
#include <ipp.hpp>


void Device::recv_loop() {
    std::cout << "[app] Channel recv thread started" << std::endl;
    const auto timeout = std::chrono::milliseconds(100);

    channel_.send(ipp::AppMsg{ipp::AppMsgConnect{}}, std::nullopt).unwrap(); // Wait forever
    std::cout << "[app] Connect signal sent" << std::endl;
    send_worker_ = std::thread([this]() { this->send_loop(); });

    while (!this->done_.load()) {
        auto result = channel_.receive(timeout);
        if (result.is_err()) {
            auto err = result.unwrap_err();
            if (err.kind == io::ErrorKind::TimedOut) {
                continue;
            } else {
                // TODO: Use fmt
                std::stringstream text;
                text << err;
                panic("IO Error: " + text.str());
            }
        }
        auto incoming = result.unwrap();
        std::visit(
            overloaded{
                [&](ipp::McuMsgDinUpdate &&din_msg) {
                    // std::cout << "Din updated: " << uint32_t(din_msg.value) << std::endl;
                    din_.value.store(din_msg.value);
                    if (din_.notify) {
                        din_.notify();
                    }
                },
                [&](ipp::McuMsgAdcData &&adc_msg) {
                    auto &adc = adcs_[adc_msg.index];
                    auto elems = adc_msg.points;
                    if (elems.size() > 0) {
                        adc.last_value.store(elems.back());
                    }

                    auto data_guard = adc.data.lock();
                    assert_true(data_guard->write_array_exact(elems.data(), elems.size()));
                    if (data_guard->size() >= adc.max_size) {
                        assert_true(adc.notify);
                        adc.notify();
                    }
                },
                [&](ipp::McuMsgDacRequest &&dac_req_msg) {
                    {
                        // Note: `send_mutex_` must be locked even if atomic is used. See `std::condition_variable` reference.
                        std::lock_guard send_guard(send_mutex_);
                        dac_.mcu_requested_count += dac_req_msg.count;
                    }
                    send_ready_.notify_all();
                },
                [&](ipp::McuMsgDebug &&debug) { //
                    std::cout << "Device: " << debug.message << std::endl;
                },
                [&](ipp::McuMsgError &&error) {
                    std::cout << "Device Error (0x" << std::hex << int(error.code) << std::dec << "): " << error.message
                              << std::endl;
                },
                [&](auto &&) { unimplemented(); },
            },
            std::move(incoming.variant) //
        );
    }

    send_ready_.notify_all();
    send_worker_.join();
}

void Device::send_loop() {
    std::cout << "[app] Channel send thread started" << std::endl;
    const auto timeout = keep_alive_period_;
    auto next_wakeup = std::chrono::steady_clock::now();
    while (!this->done_.load()) {
        std::unique_lock send_lock(send_mutex_);
        auto status = send_ready_.wait_until(send_lock, next_wakeup);
        if (status == std::cv_status::timeout) {
            channel_.send(ipp::AppMsg{ipp::AppMsgKeepAlive{}}, timeout).unwrap();
            next_wakeup = std::chrono::steady_clock::now() + keep_alive_period_;
            continue;
        }

        if (dout_.update.exchange(false)) {
            uint8_t value = dout_.value.load();
            std::cout << "[app] Send Dout value: " << uint32_t(value) << std::endl;
            channel_.send(ipp::AppMsg{ipp::AppMsgDoutUpdate{uint8_t(value)}}, timeout).unwrap();
        }
        if (dac_.mcu_requested_count.load() > 0) {
            for (;;) {
                Vec<point_t> tmp = std::move(dac_.tmp_buf);
                ipp::AppMsgDacData dac_msg;

                size_t max_count = std::min(DAC_MSG_MAX_POINTS, dac_.mcu_requested_count.load());
                size_t count = dac_.data.read_array_into(tmp, max_count);
                dac_.mcu_requested_count -= count;

                if (count > 0) {
                    dac_msg.points = std::move(tmp);
                    assert_true(dac_msg.packed_size() <= RPMSG_MAX_MSG_LEN - 1);
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
    }
}

Device::Device(std::unique_ptr<Channel> &&raw_channel) :
    channel_(std::move(raw_channel), RPMSG_MAX_MSG_LEN) //
{
    done_.store(true);
}
Device::~Device() {
    stop();
}

void Device::start() {
    done_.store(false);
    recv_worker_ = std::thread([this]() { this->recv_loop(); });
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
            std::cout << "[app:warning] Ignoring extra bits in dout_ 4-bit mask: " << value << std::endl;
        }
        {
            // Note: `send_mutex_` must be locked even if atomic is used. See `std::condition_variable` reference.
            std::lock_guard send_guard(send_mutex_);
            dout_.value.store(uint8_t(value & mask));
            dout_.update.store(true);
        }
    }
    send_ready_.notify_all();
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

void Device::write_dac(const int32_t *data, const size_t len) {
    assert_true(dac_.data.write_array_exact(data, len));
    send_ready_.notify_all();
    if (dac_.sync_ioc_request_flag) {
        dac_.ioc_requested.store(false);
        dac_.sync_ioc_request_flag();
    }
}

void Device::init_adc(uint8_t index, size_t max_size) {
    adcs_[index].max_size = max_size;
}

void Device::set_adc_callback(size_t index, std::function<void()> &&callback) {
    assert_true(index < ADC_COUNT);
    adcs_[index].notify = std::move(callback);
}

std::vector<int32_t> Device::read_adc(size_t index) {
    auto &adc = adcs_[index];

    Vec<int32_t> data;
    data.reserve(adc.max_size);
    assert_eq(data.write_array_from(*(adc.data.lock()), adc.max_size), adc.max_size);

    return data;
}

int32_t Device::read_adc_last_value(size_t index) {
    assert_true(index < ADC_COUNT);
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
        std::cout << "One-shot DAC mode set" << std::endl;
        dac_.data.set_cyclic(false);
        break;
    case DacPlaybackMode::Cyclic:
        std::cout << "Cyclic DAC mode set" << std::endl;
        dac_.data.set_cyclic(true);
        break;
    default:
        unreachable();
    }
}

void Device::set_dac_operation_state(DacOperationState) {
    std::cout << "DAC operation state changing is not supported yet" << std::endl;
}
