#include <iostream>

#include "mbbIODirect.hpp"

//===========================
//  MbbiDirect
//===========================

MbbiDirect::MbbiDirect(mbbiDirectRecord *raw) :
MbbIODirect<mbbiDirectRecord>(raw) {}

//===========================
//  MbbiDirectHandler
//===========================

MbbiDirectHandler::MbbiDirectHandler(dbCommon *raw_record) 
: Handler(raw_record) {}

MbbiDirectHandler::MbbiDirectHandler(
    dbCommon *raw_record,
    bool asyn_process
): Handler(raw_record, asyn_process) {}

void MbbiDirectHandler::readwrite() {
#ifdef RECORD_DEBUG
    MbbiDirect record((mbbiDirectRecord *)raw_record_);
    std::cout << record.name() << " MbbiDirectHandler::readwrite(). New raw value = " 
    << record.raw_value() << ", new value = " << record.value() << 
    std::endl << std::flush;
#endif
}

//===========================
//  MbboDirect
//===========================

MbboDirect::MbboDirect(mbboDirectRecord *raw) :
MbbIODirect<mbboDirectRecord>(raw) {}

//===========================
//  MbboDirectHandler
//===========================

MbboDirectHandler::MbboDirectHandler(dbCommon *raw_record) 
: Handler(raw_record) {}

MbboDirectHandler::MbboDirectHandler(
    dbCommon *raw_record,
    bool asyn_process
): Handler(raw_record, asyn_process) {}

void MbboDirectHandler::readwrite() {
#ifdef RECORD_DEBUG
    MbboDirect record((mbboDirectRecord *)raw_record_);
    std::cout << record.name() << " MbboDirectHandler::readwrite(). New raw value = " 
    << record.raw_value() << ", new value = " << record.value() << 
    std::endl << std::flush;
#endif
}