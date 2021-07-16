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

//===========================
//  MbboHandler
//===========================

MbboHandler::MbboHandler(dbCommon *raw_record) : WriteHandler(raw_record) {
    read_write_func = std::bind(&MbboHandler::write, this);
}

MbboHandler::MbboHandler(
    dbCommon *raw_record,
    std::function<callback_func_t> write_callback
): WriteHandler(raw_record, true) {
    read_write_func = std::bind(&MbboHandler::write, this);

    Record record(raw_record);
    record.set_callback(write_callback);
}

void MbboHandler::write() {
#ifdef RECORD_DEBUG
    MbboDirect record((mbboDirectRecord *)raw_record_);
    std::cout << record.name() << " MbboHandler::write(). New raw value = " 
    << record.raw_value() << ", new value = " << record.value() << 
    std::endl << std::flush;
#endif
}