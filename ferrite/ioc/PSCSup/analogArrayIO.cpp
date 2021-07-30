#include <iostream>
#include <string>

#include "analogArrayIO.hpp"

//===========================
//  Aai
//===========================

Aai::Aai(aaiRecord *raw) : 
AnalogArray<aaiRecord>(raw) {}

//===========================
//  AaiHandler
//===========================

AaiHandler::AaiHandler(dbCommon *raw_record) 
: Handler(raw_record) {}

AaiHandler::AaiHandler(
    dbCommon *raw_record,
    bool asyn_process
): Handler(raw_record, asyn_process) {}

void AaiHandler::readwrite() {
#ifdef RECORD_DEBUG
    Aai aai_record((aaiRecord *)raw_record_);

    std::cout << aai_record.name() << " AaiHandler::readwrite(). New length = " <<
    aai_record.length() << ", new data = [ ";

    for (epicsUInt32 i = 0; i < aai_record.length(); ++i) {
        std::cout << aai_record.array_data<epicsInt32>()[i] << " "; 
    }

    std::cout << "]" << std::endl << std::flush;
#endif
}

//===========================
//  Aao
//===========================

Aao::Aao(aaoRecord *raw) : 
AnalogArray<aaoRecord>(raw) {}

//===========================
//  AaoHandler
//===========================

AaoHandler::AaoHandler(dbCommon *raw_record) 
: Handler(raw_record) {}

AaoHandler::AaoHandler(
    dbCommon *raw_record,
    bool asyn_process
): Handler(raw_record, asyn_process) {}

void AaoHandler::readwrite() {
#ifdef RECORD_DEBUG
    Aao aao_record((aaoRecord *)raw_record_);

    std::cout << aao_record.name() << " AaoHandler::readwrite(). New length = " <<
    aao_record.length() << ", new data = [ ";

    for (epicsUInt32 i = 0; i < aao_record.length(); ++i) {
        std::cout << aao_record.array_data<epicsInt32>()[i] << " "; 
    }

    std::cout << "]" << std::endl << std::flush;
#endif
}