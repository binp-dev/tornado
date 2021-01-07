#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include <core/lazy_static.hpp>
#include <core/mutex.hpp>
#include <record_waveform.hpp>
#include <encoder.hpp>
#include <device.hpp>


#ifdef IOC_TEST
#include <channel_zmq.hpp>
#else // IOC_TEST
#include <channel_rpmsg.hpp>
#endif // IOC_TEST

typedef std::unique_ptr<Device> DevicePtr;

const class : public LazyStatic<Mutex<DevicePtr>> {
    virtual Mutex<DevicePtr> init_value() const override {
        std::cout << "DEVICE(:LazyStatic).init()" << std::endl;
        return Mutex(std::make_unique<Device>(
#ifdef IOC_TEST
            std::unique_ptr<Channel>(new ZmqChannel(std::move(ZmqChannel::create("tcp://127.0.0.1:8321").unwrap()))),
#else // IOC_TEST
            std::unique_ptr<Channel>(new RpmsgChannel(std::move(RpmsgChannel::create("/dev/ttyRPMSG0").unwrap()))),
#endif // IOC_TEST
            std::move(std::make_unique<LinearEncoder>(0, (1<<24) - 1, 3)),
            200,
            256
        ));
    }
} DEVICE;


class SendWaveform : public WaveformHandler {
    private:
    const Mutex<DevicePtr> *device;

    public:
    SendWaveform(
        const Mutex<DevicePtr> *device,
        WaveformRecord &record
    ) : device(device) {
        size_t dev_len = (*device->lock())->max_points();
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
        (*device->lock())->set_waveform(wf_data, wf_len);
    }
};

[[nodiscard]]
std::unique_ptr<WaveformHandler> framework_record_init_waveform(WaveformRecord &record) {
    if (strcmp(record.name(), "WAVEFORM") == 0) {
        return std::make_unique<SendWaveform>(&*DEVICE, record);
    } else {
        assert(false);
    }
}
