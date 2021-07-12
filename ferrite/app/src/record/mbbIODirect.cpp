#include <iostream>

#include "mbbIODirect.hpp"

//===========================
//  MbbiDirect
//===========================

MbbiDirect::MbbiDirect(mbbiDirectRecord *raw) :
MbbIODirect<mbbiDirectRecord>(raw) {}

void MbbiDirect::read() {
#ifdef RECORD_DEBUG
    std::cout << name() << " MbbiDirect::read()" << std::endl
    << std::flush;
#endif
}

//===========================
//  MbboDirect
//===========================

MbboDirect::MbboDirect(mbboDirectRecord *raw) :
MbbIODirect<mbboDirectRecord>(raw) {}

void MbboDirect::write() {
#ifdef RECORD_DEBUG
    std::cout << name() << " MbboDirect::write(). New raw value = " 
    << raw_value() << ", new value = " << raw()->val << std::endl << std::flush;
    
    
#endif

}