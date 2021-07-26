#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include <core/lazy_static.hpp>
#include <core/mutex.hpp>
#include <record/analogArrayIO.hpp>
#include <encoder.hpp>
#include <device.hpp>


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

class DacHandler final : public AaoHandler {
    private:
    Device *device;

    public:
    DacHandler(
        Device *device,
        Aao &record
    ) : 
    AaoHandler(record.Record::raw(), true),
    device(device) {
        size_t dev_len = device->max_points();
        size_t rec_len = record.max_length();
        if (dev_len != rec_len) {
            panic(
                "Device waveform size (" + std::to_string(dev_len) +
                ") doesn't match to the one of the record (" + std::to_string(rec_len) + ")"
            );
        }
    }
    ~DacHandler() override = default;

    virtual void readwrite() override {
        Aao record((aaoRecord *)raw_record_);

        std::cout << "DacHandler.readwrite() before" << std::endl;
        device->set_waveform(record.array_data<double>(), record.length());
        std::cout << "DacHandler.readwrite() after" << std::endl;
    }
};

void framework_init_device() {
    // Explicitly initialize device.
    *DEVICE;
}

std::unique_ptr<AaoHandler> framework_record_init_dac(Aao &record) {
    if (strcmp(record.name(), "WAVEFORM") == 0) {
        return std::make_unique<DacHandler>(&*DEVICE, record);
    } else {
        assert(false);
    }
}
