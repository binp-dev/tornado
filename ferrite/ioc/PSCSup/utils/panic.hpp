#pragma once

#include <iostream>

#include <epicsExit.h>

void panic() {
    std::cerr << "PANIC" << std::endl;
    epicsExit(1);
}
