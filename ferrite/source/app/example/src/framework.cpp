#include "framework.hpp"

#include <iostream>

#include "core/log.hpp"
#include "record/base.hpp"

void framework_init() {
    core_log_info("framework_init");
}

void framework_record_init(Record &record) {
    core_log_info("framework_record_init({})", record.name());
}

void framework_start() {
    core_log_info("framework_start");
}
