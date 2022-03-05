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

    channel.send(ipp::AppMsg{ipp::AppMsgConnect{}}, std::nullopt).unwrap(); // Wait forever
    std::cout << "[app] Connect signal sent" << std::endl;
    send_ready.notify_all();

    while (!this->done.load()) {
        auto result = channel.receive(timeout);
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
                [&](ipp::McuMsgDinVal &&din_val) {
                    // std::cout << "Din updated: " << uint32_t(din_val.value) << std::endl;
                    din.value.store(din_val.value);
                    if (din.notify) {
                        din.notify();
                    }
                },
                [&](ipp::McuMsgAdcWf &&adc_msg) {
                    auto &adc = adcs[adc_msg.index];
                    auto elems = adc_msg.elements;
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
                [&](ipp::McuMsgDacWfReq &&) {
                    dac.has_mcu_req.store(true);
                    send_ready.notify_all();
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
    send_ready.notify_all();
}

void Device::send_loop() {
    std::cout << "[app] Channel send thread started" << std::endl;

    while (!this->done.load()) {
        std::unique_lock send_lock(send_mutex);
        auto status = send_ready.wait_for(send_lock, std::chrono::milliseconds(100));
        if (status == std::cv_status::timeout) {
            continue;
        }

        if (dout.update.exchange(false)) {
            uint8_t value = dout.value.load();
            std::cout << "[app] Send Dout value: " << uint32_t(value) << std::endl;
            channel.send(ipp::AppMsg{ipp::AppMsgDoutSet{uint8_t(value)}}, std::nullopt).unwrap();
        }
        if (dac.has_mcu_req.exchange(false)) {
            auto tmp = dac.tmp_buf;
            ipp::AppMsgDacWf dac_msg;
            size_t max_count = (msg_max_len_ - dac_msg.packed_size() - 1) / sizeof(int32_t);

            dac.data.read_array_into(tmp, max_count);

            if (!tmp.empty()) {
                dac_msg.elements = std::move(tmp);

                assert_true(dac_msg.packed_size() <= msg_max_len_ - 1);
                channel.send(ipp::AppMsg{std::move(dac_msg)}, std::nullopt).unwrap();
                dac.tmp_buf = std::move(tmp);
            } else {
                dac.has_mcu_req.store(true);
            }

            if (dac.data.write_ready() && !dac.ioc_requested.load() && dac.sync_ioc_request_flag) {
                dac.sync_ioc_request_flag();
                dac.ioc_requested.store(true);
            }
        }
    }
}

Device::Device(std::unique_ptr<Channel> &&raw_channel, size_t msg_max_len) :
    msg_max_len_(msg_max_len),
    channel(std::move(raw_channel), msg_max_len) //
{
    done.store(true);
}
Device::~Device() {
    stop();
}

void Device::start() {
    done.store(false);
    send_worker = std::thread([this]() { this->send_loop(); });
    recv_worker = std::thread([this]() { this->recv_loop(); });
}

void Device::stop() {
    if (!done.load()) {
        done.store(true);
        send_worker.join();
        recv_worker.join();
    }
}

void Device::write_dout(uint32_t value) {
    {
        constexpr uint32_t mask = 0xfu;
        std::lock_guard send_guard(send_mutex);
        if ((value & ~mask) != 0) {
            std::cout << "[app:warning] Ignoring extra bits in dout 4-bit mask: " << value << std::endl;
        }
        dout.value.store(uint8_t(value & mask));
        dout.update.store(true);
    }
    send_ready.notify_all();
}

uint32_t Device::read_din() {
    return din.value.load();
}
void Device::set_din_callback(std::function<void()> &&callback) {
    din.notify = std::move(callback);
}

void Device::init_dac(const size_t max_size) {
    dac.data.reserve(max_size);
}

void Device::write_dac(const int32_t *data, const size_t len) {
    assert_true(dac.data.write_array_exact(data, len));
    send_ready.notify_all();
    if (dac.sync_ioc_request_flag) {
        dac.ioc_requested.store(false);
        dac.sync_ioc_request_flag();
    }
}

void Device::init_adc(uint8_t index, size_t max_size) {
    adcs[index].max_size = max_size;
}

void Device::set_adc_callback(size_t index, std::function<void()> &&callback) {
    assert_true(index < ADC_COUNT);
    adcs[index].notify = std::move(callback);
}

std::vector<int32_t> Device::read_adc(size_t index) {
    auto &adc = adcs[index];

    Vec<int32_t> data;
    data.reserve(adc.max_size);
    assert_eq(data.write_array_from(*(adc.data.lock()), adc.max_size), adc.max_size);

    return data;
}

int32_t Device::read_adc_last_value(size_t index) {
    assert_true(index < ADC_COUNT);
    return adcs[index].last_value.load();
}

bool Device::dac_req_flag() {
    return dac.data.write_ready();
}

void Device::set_dac_req_callback(std::function<void()> &&callback) {
    dac.sync_ioc_request_flag = std::move(callback);
}

void Device::set_dac_playback_mode(DacPlaybackMode mode) {
    switch (mode) {
    case DacPlaybackMode::OneShot:
        std::cout << "One-shot DAC mode set" << std::endl;
        dac.data.set_cyclic(false);
        break;
    case DacPlaybackMode::Cyclic:
        std::cout << "Cyclic DAC mode set" << std::endl;
        dac.data.set_cyclic(true);
        break;
    default:
        unreachable();
    }
}

void Device::set_dac_operation_state(DacOperationState) {
    std::cout << "DAC operation state changing is not supported yet" << std::endl;
}
