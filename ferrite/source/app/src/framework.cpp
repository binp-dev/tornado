#include "framework.hpp"

#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include <core/lazy_static.hpp>
#include <core/mutex.hpp>
#include <device.hpp>
#include <handlers.hpp>

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

    } else if (name == "scan_freq") {
        auto &sf_record = dynamic_cast<OutputValueRecord<int32_t> &>(record);
        sf_record.set_handler(std::make_unique<ScanFreqHandler>(*DEVICE));

    } else {
        unimplemented();
    }
}

void framework_start() {
    // Start device workers.
    DEVICE->start();
}
