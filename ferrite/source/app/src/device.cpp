#include "device.hpp"

#include <variant>
#include <cstring>

#include <core/assert.hpp>
#include <core/panic.hpp>
#include <core/cast.hpp>
#include <ipp.hpp>


void Device::recv_loop() {
    std::cout << "[app] Channel serve thread started" << std::endl;
    const auto timeout = std::chrono::milliseconds(10);

    channel.send(ipp::AppMsg{ipp::AppMsgStart{}}, std::nullopt).unwrap(); // Wait forever
    std::cout << "[app] Start signal sent" << std::endl;

    send_worker = std::thread([this]() { this->send_loop(); });

    while (!this->done.load()) {
        auto result = channel.receive(timeout);
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
        if (std::holds_alternative<ipp::McuMsgAdcVal>(incoming.variant)) {
            const auto adc_val = std::get<ipp::McuMsgAdcVal>(incoming.variant);

            // static_assert(decltype(adcs)::size() == decltype(adc_val.values)::size());
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

        } else if (std::holds_alternative<ipp::McuMsgAdcWf>(incoming.variant)) {
            const auto adc_wf_msg = std::get<ipp::McuMsgAdcWf>(incoming.variant);
            auto &adc_wf = adc_wfs[adc_wf_msg.index];
            if (!adc_wf.notify) {
                continue;
            } 

            {
                std::lock_guard<std::mutex> lock(adc_wf.mutex);
                adc_wf.wf_data.insert(adc_wf.wf_data.end(), adc_wf_msg.elements.begin(), adc_wf_msg.elements.end());

                if (adc_wf.wf_data.size() >= adc_wf.wf_max_size) {
                    adc_wf.notify();
                }
            }

        } else if (std::holds_alternative<ipp::McuMsgDacWfReq>(incoming.variant)) {
            has_dac_wf_req.store(true);
            send_ready.notify_all();

        } else if (std::holds_alternative<ipp::McuMsgDebug>(incoming.variant)) {
            std::cout << "Device: " << std::get<ipp::McuMsgDebug>(incoming.variant).message << std::endl;

        } else if (std::holds_alternative<ipp::McuMsgError>(incoming.variant)) {
            const auto &inc_err = std::get<ipp::McuMsgError>(incoming.variant);
            std::cout << "Device Error (0x" << std::hex << int(inc_err.code) << std::dec << "): " << inc_err.message
                      << std::endl;

        } else {
            unimplemented();
        }
    }

    send_ready.notify_all();
    send_worker.join();
}

void Device::send_loop() {
    std::cout << "[app] ADC req thread started" << std::endl;

    auto next_wakeup = std::chrono::system_clock::now();
    while (!this->done.load()) {
        std::unique_lock send_lock(send_mutex);
        auto status = send_ready.wait_until(send_lock, next_wakeup);

        if (status == std::cv_status::timeout) {
            // std::cout << "[app] Request ADC measurements." << std::endl;
            channel.send(ipp::AppMsg{ipp::AppMsgAdcReq{}}, std::nullopt).unwrap();
            next_wakeup = std::chrono::system_clock::now() + adc_req_period;
        }
        if (dac.update.exchange(false)) {
            int32_t value = dac.value.load();
            std::cout << "[app] Send DAC value: " << value << std::endl;
            channel.send(ipp::AppMsg{ipp::AppMsgDacSet{value}}, std::nullopt).unwrap();
        }
        if (dout.update.exchange(false)) {
            uint8_t value = dout.value.load();
            std::cout << "[app] Send Dout value: " << uint32_t(value) << std::endl;
            channel.send(ipp::AppMsg{ipp::AppMsgDoutSet{uint8_t(value)}}, std::nullopt).unwrap();
        }
        if (has_dac_wf_req.load() == true && dac_wf.wf_is_set.load() == true) {
            has_dac_wf_req.store(false);
            ipp::AppMsgDacWf dac_wf_msg;
            auto &buffer = dac_wf_msg.elements;
            size_t max_buffer_size = (msg_max_len_ - dac_wf_msg.packed_size()) / sizeof(int32_t);
            buffer.reserve(max_buffer_size);

            fill_dac_wf_msg(buffer, max_buffer_size);

            assert_true(dac_wf_msg.packed_size() < msg_max_len_);
            channel.send(ipp::AppMsg{std::move(dac_wf_msg)}, std::nullopt).unwrap();
        }
    }
}

Device::Device(DeviceChannel &&channel, size_t msg_max_len) : 
    channel(std::move(channel)),
    msg_max_len_(msg_max_len) 
{
    done.store(true);
}
Device::~Device() {
    stop();
}

void Device::start() {
    done.store(false);
    recv_worker = std::thread([this]() { this->recv_loop(); });
}

void Device::stop() {
    if (!done.load()) {
        done.store(true);
        recv_worker.join();
    }
}

void Device::write_dac(int32_t value) {
    {
        std::lock_guard send_guard(send_mutex);
        dac.value.store(value);
        dac.update.store(true);
    }
    send_ready.notify_all();
}

int32_t Device::read_adc(size_t index) {
    assert_true(index < ADC_COUNT);
    return adcs[index].value.load();
}
void Device::set_adc_callback(size_t index, std::function<void()> &&callback) {
    assert_true(index < ADC_COUNT);
    adcs[index].notify = std::move(callback);
}

void Device::set_adc_req_period(std::chrono::milliseconds period) {
    adc_req_period = period;
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

void Device::init_dac_wf(size_t wf_max_size) {
    for (size_t i = 0; i < dac_wf.wf_data.size(); ++i) {
        dac_wf.wf_data[i].resize(wf_max_size, 0.0);
    }

    dac_wf.wf_max_size = wf_max_size;
}

void Device::write_dac_wf(const int32_t *wf_data, const size_t wf_len) {
    std::lock_guard<std::mutex> guard(dac_wf.mutex);

    if (!dac_wf.wf_is_set.load()) {
        dac_wf.wf_data[0].resize(wf_len);
        std::copy(wf_data, wf_data + wf_len, dac_wf.wf_data[0].begin());
        dac_wf.wf_is_set.store(true);

        send_ready.notify_all();
    } else {
        dac_wf.wf_data[1].resize(wf_len);
        std::copy(wf_data, wf_data + wf_len, dac_wf.wf_data[1].begin());
        dac_wf.swap_ready.store(true);
    }
}

void Device::init_adc_wf(uint8_t index, size_t wf_max_size) {
     adc_wfs[index].wf_max_size = wf_max_size;
}

void Device::set_adc_wf_callback(size_t index, std::function<void()> &&callback) {
    assert_true(index < ADC_COUNT);
    adc_wfs[index].notify = std::move(callback);
}

const std::vector<int32_t> Device::read_adc_wf(size_t index) {
    auto &adc_wf = adc_wfs[index];

    std::lock_guard<std::mutex> lock(adc_wf.mutex);

    std::vector<int32_t> wf_data(adc_wf.wf_data.begin(), adc_wf.wf_data.begin() + adc_wf.wf_max_size);
    adc_wf.wf_data.erase(adc_wf.wf_data.begin(), adc_wf.wf_data.begin() + adc_wf.wf_max_size);

    return wf_data;
}

void Device::fill_dac_wf_msg(std::vector<int32_t> &msg_buff, size_t max_buffer_size) {
    copy_dac_wf_to_dac_wf_msg(msg_buff, max_buffer_size);

    while (msg_buff.size() < max_buffer_size) {
        // The message buffer is not fully filled only if the waveform has run out of data,
        // which means dac_wf.buff_position = dac_wf.wf_data[0].size().
        // So we can try to swap dac_wf buffers

        if (swap_dac_wf_buffers() == true) {
            copy_dac_wf_to_dac_wf_msg(msg_buff, max_buffer_size);
        } else {
            break;
        }
    } 
}

void Device::copy_dac_wf_to_dac_wf_msg(std::vector<int32_t> &msg_buff, size_t max_buffer_size) {
    size_t elements_to_fill = max_buffer_size - msg_buff.size();
    size_t elements_left = dac_wf.wf_data[0].size() - dac_wf.buff_position;
    if (elements_left < elements_to_fill) {
        elements_to_fill = elements_left;
    }

    auto dac_wf_buff_pos_iter = dac_wf.wf_data[0].begin() + dac_wf.buff_position;
    msg_buff.insert(msg_buff.end(), dac_wf_buff_pos_iter, dac_wf_buff_pos_iter + elements_to_fill);
    
    dac_wf.buff_position += elements_to_fill;
}

bool Device::swap_dac_wf_buffers() {
    dac_wf.buff_position = 0;
    std::lock_guard<std::mutex> guard(dac_wf.mutex);
    
    if (dac_wf.swap_ready.exchange(false) == true) {
        dac_wf.wf_data[0].swap(dac_wf.wf_data[1]);
        return true;
    } else {
        if (cyclic_dac_wf_output) {
            return true;
        }

        dac_wf.wf_is_set.store(false);
    }

    return false;
}