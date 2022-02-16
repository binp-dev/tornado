#include "framework.hpp"

#include <iostream>

#include "record/base.hpp"


void framework_init() {
    std::cout << "[app_lib] framework_init" << std::endl;
}

void framework_record_init(Record &) {
    std::cout << "[app_lib] framework_record_init" << std::endl;
}

void framework_start() {
    std::cout << "[app_lib] framework_start" << std::endl;
}
