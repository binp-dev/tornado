#include <iostream>

#include "analogArrayIO.hpp"

//===========================
//  AnalogArrayInput
//===========================

AnalogArrayInput::AnalogArrayInput(aaiRecord *raw) : AnalogArray<aaiRecord>(raw) {}

void AnalogArrayInput::read() {
    std::cout << "AnalogArrayInput::read()" << std::endl;
}