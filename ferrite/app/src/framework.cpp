#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include <core/lazy_static.hpp>
#include <core/mutex.hpp>
#include <record/waveform.hpp>
#include <encoder.hpp>
#include <device.hpp>


#ifdef FAKEDEV
#include <channel/zmq.hpp>
#else // FAKEDEV
#include <channel/rpmsg.hpp>
#endif // FAKEDEV

typedef std::shared_ptr<Device> DevicePtr;

const class : public LazyStatic<DevicePtr> {
    virtual DevicePtr init_value() const override {
        std::cout << "DEVICE(:LazyStatic).init()" << std::endl;

        return std::make_shared<Device>(
#ifdef FAKEDEV
            std::unique_ptr<Channel>(new ZmqChannel(std::move(ZmqChannel::create("tcp://127.0.0.1:8321").unwrap()))),
#else // FAKEDEV
            std::unique_ptr<Channel>(new RpmsgChannel(std::move(RpmsgChannel::create("/dev/ttyRPMSG0").unwrap()))),
#endif // FAKEDEV
            std::move(std::make_unique<LinearEncoder>(0, (1<<24) - 1, 3)),
            200,
            256
        );
    }
} DEVICE;

class SendWaveform : public WaveformHandler {
    private:
    const DevicePtr device;

    public:
    SendWaveform(
        const DevicePtr device,
        WaveformRecord &record
    ) : device(device) {
        size_t dev_len = device->max_points();
        size_t rec_len = record.waveform_max_length();
        if (dev_len != rec_len) {
            panic(
                "Device waveform size (" + std::to_string(dev_len) +
                ") doesn't match to the one of the record (" + std::to_string(rec_len) + ")"
            );
        }
    }
    ~SendWaveform() override = default;

    void read(WaveformRecord &record) override {
        auto [wf_data, wf_len] = record.waveform_slice<double>();
        device->set_waveform(wf_data, wf_len);
    }
};

void framework_init_device() {
    // Explicitly initialize device.
    *DEVICE;
}

std::unique_ptr<WaveformHandler> framework_record_init_waveform(WaveformRecord &record) {
    if (strcmp(record.name(), "WAVEFORM") == 0) {
        return std::make_unique<SendWaveform>(*DEVICE, record);
    } else {
        assert(false);
    }
}
