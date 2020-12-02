#pragma once

#include <iostream>
#include <string>

#ifdef EPICS
#include <epicsExit.h>
#endif // EPICS

[[noreturn]] inline void panic(const std::string &message = "") {
    std::cerr << "PANIC: " << message << "." << std::endl;
#ifdef EPICS
    epicsExit(1);
#else // !EPICS
    std::abort();
#endif // EPICS
}
