#include "framework.hpp"

#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include <core/lazy_static.hpp>
#include <core/mutex.hpp>
#include <record/value.hpp>
#include <device.hpp>

#ifdef FAKEDEV
#include <channel/zmq.hpp>
#else // FAKEDEV
#include <channel/rpmsg.hpp>
#endif // FAKEDEV

void init_device(MaybeUninit<Device> &mem) {
    std::cout << "DEVICE(:LazyStatic).init()" << std::endl;

    const size_t message_max_length = 256;
    std::unique_ptr<Channel> channel =
#ifdef FAKEDEV
        std::make_unique<ZmqChannel>(std::move(
            ZmqChannel::create("tcp://127.0.0.1:8321", message_max_length).unwrap()
        ))
#else // FAKEDEV
        std::make_unique<RpmsgChannel>(std::move(
            RpmsgChannel::create("/dev/ttyRPMSG0", message_max_length).unwrap()
        ))
#endif // FAKEDEV
    ;
    mem.init_in_place(std::move(channel));
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
        return false;
    }
};

class AdcHandler final : public InputValueHandler<int32_t> {
private:
    Device &device_;
    uint8_t index_;

public:
    AdcHandler(Device &device, uint8_t index) : device_(device), index_(index) {}

    virtual void read(InputValueRecord<int32_t> &record) override {
        record.set_value(device_.read_adc(index_));
    }

    virtual void set_read_request(InputValueRecord<int32_t> &, std::function<void()> &&callback) override {
        device_.set_adc_callback(index_, std::move(callback));
    }

    virtual bool is_async() const override {
        return false;
    }
};

class DoutHandler final : public OutputValueHandler<uint32_t> {
private:
    Device &device_;

public:
    DoutHandler(Device &device) : device_(device) {}

    virtual void write(OutputValueRecord<uint32_t> &record) override {
        device_.write_dout(record.value());
    }

    virtual bool is_async() const override {
        return false;
    }
};

class DinHandler final : public InputValueHandler<uint32_t> {
private:
    Device &device_;

public:
    DinHandler(Device &device) : device_(device) {}

    virtual void read(InputValueRecord<uint32_t> &record) override {
        record.set_value(device_.read_din());
    }

    virtual void set_read_request(InputValueRecord<uint32_t> &, std::function<void()> &&callback) override {
        device_.set_din_callback(std::move(callback));
    }

    virtual bool is_async() const override {
        return false;
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
    } else if (name == "do0") {
        auto &do_record = dynamic_cast<OutputValueRecord<uint32_t> &>(record);
        do_record.set_handler(std::make_unique<DoutHandler>(*DEVICE));
    } else if (name == "di0") {
        auto &di_record = dynamic_cast<InputValueRecord<uint32_t> &>(record);
        di_record.set_handler(std::make_unique<DinHandler>(*DEVICE));
    } else {
        unimplemented();
    }
}
