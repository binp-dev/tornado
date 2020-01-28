#pragma once

#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <mutex>
#include <atomic>

#include <utils/mutex.hpp>
#include <utils/lazy_static.hpp>
#include <record/waveform_record.hpp>
#include <encoder.hpp>
#include <device.hpp>


#ifdef TEST
#include <channel/zmq_channel.hpp>
#else // TEST
#include <channel/rpmsg_channel.hpp>
#endif // TEST


class : public LazyStatic<Mutex<Device>> {
    std::unique_ptr<Mutex<Device>> init() override {
        //std::cout << "DEVICE(:LazyStatic).init()" << std::endl;
        return std::make_unique<Mutex<Device>>(
            #ifdef TEST
                std::move(std::unique_ptr<Channel>(new ZmqChannel("tcp://127.0.0.1:8321"))),
            #else // TEST
                #error "unimplemented"
            #endif // TEST
            std::move(std::make_unique<LinearEncoder>(0, (1<<24) - 1, 3)),
            1024,
            256
        );
    }
} DEVICE;


class SendWaveform : public WaveformHandler {
    private:
    std::shared_ptr<Mutex<Device>> device;

    public:
    SendWaveform(
        std::shared_ptr<Mutex<Device>> device,
        WaveformRecord &record
    ) : device(device) {
        assert(device->lock()->max_points() == record.waveform_max_length());
    }
    ~SendWaveform() override = default;

    void read(WaveformRecord &record) override {
        device->lock()->set_waveform(record.waveform_slice<double>());
    }
};


std::unique_ptr<WaveformHandler> framework_record_init_waveform(WaveformRecord &record) {
    if (strcmp(record.name(), "WAVEFORM") == 0) {
        return std::make_unique<SendWaveform>(DEVICE.get(), record);
    } else {
        assert(false);
    }
}


#ifdef UNITTEST
#include <catch/catch.hpp>
TEST_CASE( "Dummy test", "[dummy]" ) {
    REQUIRE(1 + 1 == 2);
}
#endif //UNITTEST
