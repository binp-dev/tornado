#pragma once

#include <iostream>
#include <string>

#ifdef EPICS
#include <epicsExit.h>
#endif // EPICS

[[noreturn]] void panic(const std::string &message = "") {
    std::cerr << "PANIC: " << message << "." << std::endl;
#ifdef EPICS
    epicsExit(1);
#else // !EPICS
    std::abort();
#endif // EPICS
}

void assert_(bool value) {
    if (!value) {
        panic("Assertion failed");
    }
}
