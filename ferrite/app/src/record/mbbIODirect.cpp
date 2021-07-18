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

// MbboDirect::MbboDirect(mbboDirectRecord *raw) :
// MbbIODirect<mbboDirectRecord>(raw) {}

// void MbboDirect::write() {
// #ifdef RECORD_DEBUG
//     std::cout << name() << " MbboDirect::write(). New raw value = " 
//     << raw_value() << ", new value = " << raw()->val << std::endl << std::flush;
// #endif
// }

//===========================
//  MbboDirectHandler
//===========================

MbboDirectHandler::MbboDirectHandler(dbCommon *raw_record) 
: WriteHandler(raw_record) {
    // read_write_func = std::bind(&MbboDirectHandler::write, this);
}

MbboDirectHandler::MbboDirectHandler(
    dbCommon *raw_record,
    bool asyn_process
): WriteHandler(raw_record, asyn_process) {
    // read_write_func = std::bind(&MbboDirectHandler::write, this);

    // Record record(raw_record);
    // record.set_callback(write_callback);
}

void MbboDirectHandler::write() {
#ifdef RECORD_DEBUG
    MbbIODirect<mbboDirectRecord> record((mbboDirectRecord *)raw_record_);
    std::cout << record.name() << " MbboDirectHandler::write(). New raw value = " 
    << record.raw_value() << ", new value = " << record.value() << 
    std::endl << std::flush;
#endif
}