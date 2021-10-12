#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstring>
#include <cassert>

#include <core/assert.hpp>
#include <core/panic.hpp>
#include <core/mpsc.hpp>
#include <channel/base.hpp>
#include <encoder.hpp>
#include <ipp.hpp>


class Device {
private:
    const size_t _max_points;
    const size_t _max_transfer;
    std::vector<uint32_t> out_waveforms[3];
    std::atomic_bool swap_ready;
    std::mutex swap_mutex;
    size_t waveform_pos = 0;

    std::vector<uint32_t> in_waveforms[2];
    int active_input_buff = 0;
    size_t input_wf_pos = 0;
    std::function<void()> request_in_wf_processing;
    std::mutex *in_wf_mutex;

    struct AdcValue {
        uint8_t channel_no;
        uint32_t value;
    };

    // Shared data
    std::atomic_bool done;

    // Device support data
    std::mutex mutex; // FIXME: Allow concurrent operation.
    std::thread worker;
    std::optional<mpsc::Receiver<AdcValue>> adc_out;

    // Worker data
    std::unique_ptr<Channel> channel;
    std::optional<mpsc::Sender<AdcValue>> adc_in;

private:
    void fill(uint32_t **dst, size_t *dst_size) {
        size_t elements_to_fill = *dst_size;
        size_t elements_left = out_waveforms[0].size() - waveform_pos;
        if (elements_left < elements_to_fill) {
            elements_to_fill = elements_left;
        }
        
        std::memcpy(*dst, out_waveforms[0].data() + waveform_pos, elements_to_fill*sizeof(uint32_t));
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

    size_t fill_until_full(uint32_t *data, size_t size) {
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

    void fill_input_wf_buffer(const std::vector<uint32_t> &new_wf_data) {
        size_t elements_to_fill = new_wf_data.size();
        size_t elements_left = max_points() - input_wf_pos;
        size_t nonfitting_elments = (elements_to_fill > elements_left) ? (elements_to_fill - elements_left) : 0;
        if (elements_left < elements_to_fill) {
            elements_to_fill = elements_left;
        }

        std::memcpy(
            in_waveforms[active_input_buff].data() + input_wf_pos,
            new_wf_data.data(),
            elements_to_fill*sizeof(uint32_t)
        );
        input_wf_pos += elements_to_fill;
        
        if (input_wf_pos == max_points()) {
            std::lock_guard<std::mutex> guard(*in_wf_mutex);
            active_input_buff = (active_input_buff + 1) % 2;
            input_wf_pos = 0;

            if (nonfitting_elments != 0) {
                std::memcpy(
                    in_waveforms[active_input_buff].data() + input_wf_pos,
                    new_wf_data.data(),
                    nonfitting_elments*sizeof(uint32_t)
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
                adc_in->send(AdcValue{adc_val.index, adc_val.value});

            } else if (std::holds_alternative<ipp::McuMsgDebug>(incoming.variant)) {
                std::cout << "Device: " << std::get<ipp::McuMsgDebug>(incoming.variant).message << std::endl;

            } else if (std::holds_alternative<ipp::McuMsgWfReq>(incoming.variant)) {
                ipp::AppMsgWfData outgoing;
                auto &buffer = outgoing.data;

                // const size_t min_len = ipp::MsgAppAny{ipp::MsgAppWfData{}}.length();
                const size_t min_len = outgoing.packed_size();
                assert_true(min_len < _max_transfer);
                size_t msg_remaining_size = _max_transfer - min_len;
                size_t max_points_in_msg = msg_remaining_size / 3; // sizeof(int24) == 3
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

    // Device support methods
public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;

    Device(
        std::unique_ptr<Channel> channel,
        size_t max_points,
        size_t max_transfer
    ) : 
        _max_points(max_points),
        _max_transfer(max_transfer),
        swap_ready(false),
        channel(std::move(channel)) 
    {
        auto [in, out] = mpsc::make_channel<AdcValue>();
        adc_in = std::move(in);
        adc_out = std::move(out);

        std::lock_guard<std::mutex> device_guard(mutex);

        for (int i = 0; i < 3; ++i) {
            out_waveforms[i].resize(max_points, 0.0);
        }

        for (int i = 0; i < 2; ++i) {
            in_waveforms[i].resize(max_points, 0.0);
        }
        active_input_buff = 0;

        done.store(false);
        worker = std::thread([this]() { this->serve_loop(); });
    }
    
    virtual ~Device() {
        done.store(true);
        worker.join();
    }

    void write_dac(uint32_t value) {
        std::lock_guard<std::mutex> device_guard(mutex);

        ipp::AppMsgDacSet outgoing{value};
        channel->send(ipp::AppMsg{std::move(outgoing)}, std::nullopt).unwrap();
    }

    uint32_t read_adc(uint8_t index) {
        std::lock_guard<std::mutex> device_guard(mutex);

        assert_false(adc_out->try_receive().has_value());

        ipp::AppMsgAdcReq outgoing{index};
        channel->send(ipp::AppMsg{std::move(outgoing)}, std::nullopt).unwrap();

        const auto adc_value = adc_out->receive();
        assert_true(adc_value.has_value());
        assert_eq(adc_value->channel_no, index);
        return adc_value->value;
    }

    void write_waveform(const uint32_t *wf_data, const size_t wf_len) {
        std::lock_guard<std::mutex> device_guard(mutex);

        assert(wf_len <= max_points());
        if (!swap_ready.load()) {
            std::copy(wf_data, wf_data + wf_len, out_waveforms[1].begin());
            swap_ready.store(true);
        } else {
            std::copy(wf_data, wf_data + wf_len, out_waveforms[2].begin());
            { 
                std::lock_guard<std::mutex> guard(swap_mutex);
                // std::swap(out_waveforms[1], out_waveforms[2]);
                out_waveforms[2].swap(out_waveforms[1]);
                swap_ready.store(true);
            }
        }
    }

    const std::vector<uint32_t> & read_waveform() {
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

    void set_input_wf_mutex(std::mutex *mutex) {
        this->in_wf_mutex = mutex;
    }
};
