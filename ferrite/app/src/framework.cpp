#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include <core/lazy_static.hpp>
#include <core/mutex.hpp>
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
        std::move(std::make_unique<LinearEncoder>(0, (1<<24) - 1, 3)),
        200,
        message_max_length
    );
}

/// We use LazyStatic to initialize global Device without global constructor. 
LazyStatic<Device, init_device> DEVICE = {};
static_assert(std::is_pod_v<decltype(DEVICE)>);

class DacHandler final : public InputArrayHandler<double> {
    private:
    Device &device;

    public:
    DacHandler(
        Device &device,
        InputArrayRecord<double> &record
    ) :
        device(device)
    {
        assert_eq(device.max_points(), record.max_length());
    }

    virtual void read(InputArrayRecord<double> &record) override {
        std::cout << "DacHandler.read() before" << std::endl;
        device.set_waveform(record.data(), record.length());
        std::cout << "DacHandler.read() after" << std::endl;
    }

    virtual bool is_async() const override {
        return true;
    }
};

void framework_init() {
    // Explicitly initialize device.
    *DEVICE;
}

void framework_record_init(Record &record) {
    assert_eq(record.name(), "WAVEFORM");
    auto &waveform_record = dynamic_cast<InputArrayRecord<double> &>(record);
    waveform_record.set_handler(std::make_unique<DacHandler>(*DEVICE, waveform_record));
}
