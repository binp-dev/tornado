#include <framework.hpp>

#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include <core/lazy_static.hpp>

#include <device.hpp>
#include <handlers.hpp>
#include <external.hpp>

void init_device(MaybeUninit<Device> &mem) {
    std::cout << "DEVICE(:LazyStatic).init()" << std::endl;

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

    } else if (name.rfind("aai", 0) == 0) { // name.startswith("aai")
        const auto index_str = name.substr(3);
        uint8_t index = std::stoi(std::string(index_str));
        auto &aai_record = dynamic_cast<InputArrayRecord<double> &>(record);
        aai_record.set_handler(std::make_unique<AdcWfHandler>(*DEVICE, aai_record, index));

    } else if (name == "aao0") {
        auto &aao_record = dynamic_cast<OutputArrayRecord<double> &>(record);
        aao_record.set_handler(std::make_unique<DacWfHandler>(*DEVICE, aao_record));

    } else if (name == "aao0_request") {
        auto &aao_req_record = dynamic_cast<InputValueRecord<bool> &>(record);
        aao_req_record.set_handler(std::make_unique<WfReqHandler>(*DEVICE));

    } else if (name == "aao0_cyclic") {
        auto &specific_record = dynamic_cast<OutputValueRecord<bool> &>(record);
        specific_record.set_handler(std::make_unique<DacPlaybackModeHandler>(*DEVICE));

    } else if (name == "aao0_running") {
        auto &specific_record = dynamic_cast<OutputValueRecord<bool> &>(record);
        specific_record.set_handler(std::make_unique<DacOpStateHandler>(*DEVICE));

    } else if (name == "stats_reset") {
        auto &specific_record = dynamic_cast<OutputValueRecord<bool> &>(record);
        specific_record.set_handler(std::make_unique<StatsResetHandler>(*DEVICE));

    } else {
        unimplemented();
    }
}

void framework_start() {
    // Start device workers.
    DEVICE->start();
}
