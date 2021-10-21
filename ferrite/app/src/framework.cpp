#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <mutex>

#include <core/lazy_static.hpp>
#include <core/mutex.hpp>
#include <record/value.hpp>
#include <record/array.hpp>
#include <encoder.hpp>
#include <device.hpp>
#include "framework.hpp"

#ifdef FAKEDEV
#include <channel/zmq.hpp>
#else // FAKEDEV
#include <channel/rpmsg.hpp>
#endif // FAKEDEV

void init_device(MaybeUninit<Device> &mem) {
    std::cout << "DEVICE(:LazyStatic).init()" << std::endl;

    const size_t message_max_length = 256;
    mem.init_in_place(
#ifdef FAKEDEV
        std::unique_ptr<Channel>(new ZmqChannel(std::move(
            ZmqChannel::create("tcp://127.0.0.1:8321", message_max_length).unwrap()
        ))),
#else // FAKEDEV
        std::unique_ptr<Channel>(new RpmsgChannel(std::move(
            RpmsgChannel::create("/dev/ttyRPMSG0", message_max_length).unwrap()
        ))),
#endif // FAKEDEV
        200,
        message_max_length
    );
}

/// We use LazyStatic to initialize global Device without global constructor. 
LazyStatic<Device, init_device> DEVICE = {};
static_assert(std::is_pod_v<decltype(DEVICE)>);

class DacHandler final : public OutputValueHandler<int32_t> {
private:
    Device &device_;

public:
    DacHandler(Device &device) : device_(device) {}

    virtual void write(OutputValueRecord<int32_t> &record) override {
        device_.write_dac(record.value());
    }

    virtual bool is_async() const override {
        return true;
    }
};

class AdcHandler final : public InputValueHandler<int32_t> {
private:
    Device &device_;
    uint8_t channel_;

public:
    AdcHandler(Device &device, uint8_t channel) : device_(device), channel_(channel) {}

    virtual void read(InputValueRecord<int32_t> &record) override {
        record.set_value(device_.read_adc(channel_));
    }

    virtual void set_read_request(InputValueRecord<int32_t> &, std::function<void()> &&) override {
        unimplemented();
    }

    virtual bool is_async() const override {
        return true;
    }
};

class DacWfHandler final : public OutputArrayHandler<int32_t> {
private:
    Device &device;

public:
    DacWfHandler(
        Device &device,
        OutputArrayRecord<int32_t> &record
    ) :
        device(device)
    {
        assert_eq(device.max_points(), record.max_length());
    }

    virtual void write(OutputArrayRecord<int32_t> &record) override {
        device.write_waveform(record.data(), record.length());
    }

    virtual bool is_async() const override {
        return true;
    }
};

class AdcWfHandler final : public InputArrayHandler<int32_t> {
private:
    Device &device;
    std::mutex input_wf_mutex;
public:
    AdcWfHandler(
        Device &device,
        InputArrayRecord<int32_t> &record
    ) :
        device(device)
    {
        assert_eq(device.max_points(), record.max_length());
        device.set_input_wf_mutex(&input_wf_mutex);
    }

    virtual void read(InputArrayRecord<int32_t> &record) override {
        std::lock_guard<std::mutex> guard(input_wf_mutex);
        auto input_wf = device.read_waveform();
        record.set_data(input_wf.data(), input_wf.size());
    }

    virtual bool is_async() const override {
        return true;
    }

    virtual void set_read_request(InputArrayRecord<int32_t> &record, std::function<void()> &&callback) override {
        callback();
        //device.set_input_wf_ready_callback(std::move(callback));
    }
};

void framework_init() {
    // Explicitly initialize device.
    *DEVICE;
}

void framework_record_init(Record &record) {
    const auto name = record.name();
    std::cout << "Initializing record '" << name << "'" << std::endl;
    if (name == "ao0") {
        auto &ao_record = dynamic_cast<OutputValueRecord<int32_t> &>(record);
        ao_record.set_handler(std::make_unique<DacHandler>(*DEVICE));
    } else if (name.rfind("ai", 0) == 0) { // name.startswith("ai")
        const auto index_str = name.substr(2);
        uint8_t index = std::stoi(std::string(index_str));
        auto &ai_record = dynamic_cast<InputValueRecord<int32_t> &>(record);
        ai_record.set_handler(std::make_unique<AdcHandler>(*DEVICE, index));
    } else if (name.rfind("di", 0) == 0 || name.rfind("do", 0) == 0) {
        // TODO: Handle digital input/output
    } else if (record.name().rfind("aao", 0) == 0) {
        auto &aao_record = dynamic_cast<OutputArrayRecord<int32_t> &>(record);
        aao_record.set_handler(std::make_unique<DacWfHandler>(*DEVICE, aao_record));
    } else if (record.name().rfind("aai", 0) == 0) {
        auto &aai_record = dynamic_cast<InputArrayRecord<int32_t> &>(record);
        aai_record.set_handler(std::make_unique<AdcWfHandler>(*DEVICE, aai_record)); 

    } else {
        unimplemented();
    }
}