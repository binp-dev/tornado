#include <iostream>
#include <string>

#include "analogArrayIO.hpp"

//===========================
//  AnalogArrayInput
//===========================

AnalogArrayInput::AnalogArrayInput(aaiRecord *raw) : 
AnalogArray<aaiRecord>(raw) {}

void AnalogArrayInput::read() {
#ifdef RECORD_DEBUG
    std::cout << name() << " AnalogArrayInput::read()" << std::endl
    << std::flush;
#endif
}

//===========================
//  AnalogArrayOutput
//===========================

AnalogArrayOutput::AnalogArrayOutput(aaoRecord *raw) : 
AnalogArray<aaoRecord>(raw) {}

void AnalogArrayOutput::write() {
#ifdef RECORD_DEBUG
    std::cout << name() << " AnalogArrayOutput::write(). New length = " <<
    get_length() << ", new data = [ ";

    for (unsigned long i = 0; i < get_length(); ++i) {
        std::cout << ((int *)get_raw_data())[i] << " "; 
    }

    std::cout << " ]" << std::endl << std::flush;
#endif
}