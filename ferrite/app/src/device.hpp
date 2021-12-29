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

    const size_t _max_points;
    const size_t _max_transfer;
    
    std::vector<int32_t> dac_waveforms[3];
    std::atomic_bool swap_ready;
    std::mutex swap_mutex;
    size_t waveform_pos = 0;

    std::vector<int32_t> in_waveforms[2];
    bool input_wf_is_set = false;
    int active_input_buff = 0;
    size_t input_wf_pos = 0;
    std::function<void()> request_in_wf_processing;
    std::mutex *in_wf_mutex;

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
    void fill(int32_t **dst, size_t *dst_size) {
        size_t elements_to_fill = *dst_size;
        size_t elements_left = out_waveforms[0].size() - waveform_pos;
        if (elements_left < elements_to_fill) {
            elements_to_fill = elements_left;
        }
        
        std::memcpy(*dst, out_waveforms[0].data() + waveform_pos, elements_to_fill*sizeof(int32_t));
        *dst += elements_to_fill;
        *dst_size -= elements_to_fill;
        waveform_pos += elements_to_fill;
    }

    bool try_swap() {
        if (waveform_pos >= out_waveforms[0].size()) {
            if (swap_ready.load()) {
                std::lock_guard<std::mutex> guard(swap_mutex);
                out_waveforms[0].swap(out_waveforms[1]);
                swap_ready.store(false);
            }
            waveform_pos = 0;
            return true;
        } else {
            return false;
        }
    }

    size_t fill_until_full(int32_t *data, size_t size) {
        size_t orig_size = size;

        try_swap();
        fill(&data, &size);

        while (true) {
            if (!try_swap()) {
                break;
            }
            fill(&data, &size);
        }

        return orig_size - size;
    }

    void fill_input_wf_buffer(const std::vector<int32_t> &new_wf_data) {
        size_t elements_to_fill = new_wf_data.size();
        size_t elements_left = max_points() - input_wf_pos;
        size_t nonfitting_elments = (elements_to_fill > elements_left) ? (elements_to_fill - elements_left) : 0;
        if (elements_left < elements_to_fill) {
            elements_to_fill = elements_left;
        }

        std::memcpy(
            in_waveforms[active_input_buff].data() + input_wf_pos,
            new_wf_data.data(),
            elements_to_fill*sizeof(int32_t)
        );
        input_wf_pos += elements_to_fill;
        
        if (input_wf_pos == max_points()) {
            std::lock_guard<std::mutex> guard(*in_wf_mutex);
            active_input_buff = (active_input_buff + 1) % 2;
            input_wf_pos = 0;

            if (nonfitting_elments != 0) {
                std::memcpy(
                    in_waveforms[active_input_buff].data() + input_wf_pos,
                    new_wf_data.data() + elements_to_fill,
                    nonfitting_elments*sizeof(int32_t)
                );
                input_wf_pos += nonfitting_elments;
            }
        }
    }

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

                //static_assert(decltype(adcs)::size() == decltype(adc_val.values)::size());
                for (size_t i = 0; i < ADC_COUNT; ++i) {
                    adcs[i].value.store(adc_val.values[i]);

                    if (adcs[i].callback) {
                        adcs[i].callback();
                    }
                }
            } else if (std::holds_alternative<ipp::McuMsgDebug>(incoming.variant)) {
                std::cout << "Device: " << std::get<ipp::McuMsgDebug>(incoming.variant).message << std::endl;
            } else if (std::holds_alternative<ipp::McuMsgWfReq>(incoming.variant)) {
                if (!input_wf_is_set) {
                    continue;
                }

                ipp::AppMsgWfData outgoing;
                auto &buffer = outgoing.data;

                // const size_t min_len = ipp::MsgAppAny{ipp::MsgAppWfData{}}.length();
                const size_t min_len = outgoing.packed_size();
                assert_true(min_len < _max_transfer);
                size_t msg_remaining_size = _max_transfer - min_len;
                size_t max_points_in_msg = msg_remaining_size / sizeof(int32_t);
                buffer.resize(max_points_in_msg, 0);
                buffer.resize(fill_until_full(buffer.data(), buffer.size()));

                assert_true(outgoing.packed_size() < _max_transfer);
                channel->send(ipp::AppMsg{std::move(outgoing)}, std::nullopt).unwrap();
            } else if (std::holds_alternative<ipp::McuMsgWfData>(incoming.variant)) {
                const auto input_msg = std::get<ipp::McuMsgWfData>(incoming.variant);
                assert_true(input_msg.data.size() < _max_points);

                auto old_active_buff = active_input_buff;
                fill_input_wf_buffer(input_msg.data);
                
                if (old_active_buff != active_input_buff) {
                    request_in_wf_processing();
                }
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

    Device(
        std::unique_ptr<Channel> channel,
        std::chrono::milliseconds period,
        size_t max_points,
        size_t max_transfer
    ) : 
        channel(std::move(channel)),
        adc_req_period(period),
        _max_points(max_points),
        _max_transfer(max_transfer),
        swap_ready(false)
    {
        std::lock_guard<std::mutex> device_guard(device_mutex);

        for (int i = 0; i < 3; ++i) {
            out_waveforms[i].resize(max_points, 0.0);
        }

        for (int i = 0; i < 2; ++i) {
            in_waveforms[i].resize(max_points, 0.0);
        }
        active_input_buff = 0;

        done.store(false);
        recv_worker = std::thread([this]() { this->serve_loop(); });
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

    void write_waveform(const int32_t *wf_data, const size_t wf_len) {
        std::lock_guard<std::mutex> device_guard(device_mutex);

        input_wf_is_set = true;

        assert(wf_len <= max_points());
        if (!swap_ready.load()) {
            std::copy(wf_data, wf_data + wf_len, out_waveforms[1].begin());
            swap_ready.store(true);
        } else {
            std::copy(wf_data, wf_data + wf_len, out_waveforms[2].begin());
            { 
                std::lock_guard<std::mutex> guard(swap_mutex);
                out_waveforms[2].swap(out_waveforms[1]);
                swap_ready.store(true);
            }
        }
    }

    const std::vector<int32_t> & read_waveform() {
        return in_waveforms[(active_input_buff + 1) % 2];
    }

    size_t max_points() const {
        return _max_points;
    }

    size_t max_transfer() const {
        return _max_transfer;
    }

    void set_input_wf_ready_callback(std::function<void()> &&callback) {
        request_in_wf_processing = std::move(callback);
    }

    void set_input_wf_mutex(std::mutex *in_wf_mutex) {
        this->in_wf_mutex = in_wf_mutex;
    }
};
