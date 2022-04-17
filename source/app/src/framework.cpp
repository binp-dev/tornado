#include <framework.hpp>

#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include <core/convert.hpp>
#include <core/lazy_static.hpp>
#include <core/log.hpp>

#include <device.hpp>
#include <handlers.hpp>
#include <external.hpp>

using namespace core;

void init_device(MaybeUninit<Device> &mem) {
    core_log_info("LazyStatic: Device::init()");
    mem.init_in_place(make_device_channel(), max_message_length());
}

/// We use LazyStatic to initialize global Device without global constructor.
/// NOTE: Must subject to [constant initialization](https://en.cppreference.com/w/cpp/language/constant_initialization).
LazyStatic<Device, init_device> DEVICE = {};


void framework_init() {
    // Explicitly initialize device.
    *DEVICE;
}

void framework_record_init(Record &record) {
    const auto name = record.name();
    core_log_debug("Init record: {}", name);

    if (name == "ao0") {
        core::downcast<OutputValueRecord<int32_t>>(record).unwrap().get().set_handler( //
            std::make_unique<DacHandler>(*DEVICE));

    } else if (name.rfind("ai", 0) == 0) { // name.startswith("ai")
        const auto index_str = name.substr(2);
        uint8_t index = std::stoi(std::string(index_str));
        core::downcast<InputValueRecord<int32_t>>(record).unwrap().get().set_handler( //
            std::make_unique<AdcHandler>(*DEVICE, index));

    } else if (name == "do0") {
        core::downcast<OutputValueRecord<uint32_t>>(record).unwrap().get().set_handler( //
            std::make_unique<DoutHandler>(*DEVICE));

    } else if (name == "di0") {
        core::downcast<InputValueRecord<uint32_t>>(record).unwrap().get().set_handler( //
            std::make_unique<DinHandler>(*DEVICE));

    } else if (name.rfind("aai", 0) == 0) { // name.startswith("aai")
        const auto index_str = name.substr(3);
        uint8_t index = std::stoi(std::string(index_str));
        auto &current_record = core::downcast<InputArrayRecord<double>>(record).unwrap().get();
        current_record.set_handler(std::make_unique<AdcWfHandler>(*DEVICE, current_record, index));

    } else if (name == "aao0") {
        auto &current_record = core::downcast<OutputArrayRecord<double>>(record).unwrap().get();
        current_record.set_handler(std::make_unique<DacWfHandler>(*DEVICE, current_record));

    } else if (name == "aao0_request") {
        core::downcast<InputValueRecord<bool>>(record).unwrap().get().set_handler( //
            std::make_unique<WfReqHandler>(*DEVICE));

    } else if (name == "aao0_cyclic") {
        core::downcast<OutputValueRecord<bool>>(record).unwrap().get().set_handler( //
            std::make_unique<DacPlaybackModeHandler>(*DEVICE));

    } else if (name == "aao0_running") {
        core::downcast<OutputValueRecord<bool>>(record).unwrap().get().set_handler( //
            std::make_unique<DacOpStateHandler>(*DEVICE));

    } else if (name == "stats_reset") {
        core::downcast<OutputValueRecord<bool>>(record).unwrap().get().set_handler( //
            std::make_unique<StatsResetHandler>(*DEVICE));

    } else {
        core_log_fatal("Unexpected record: {}", name);
        core_unimplemented();
    }
}

void framework_start() {
    // Start device workers.
    DEVICE->start();
}
