#pragma once

#include <iostream>
#include <string>

#include <epicsExit.h>

void panic(const std::string &message = "") {
    std::cerr << "PANIC: " << message << "." << std::endl;
    epicsExit(1);
}

void assert_(bool value, const std::string &message = "") {
    if (!value) {
        panic("Assertion failed");
    }
}
