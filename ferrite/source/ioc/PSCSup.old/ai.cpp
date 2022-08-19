#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <aiRecord.h>

#include "ai.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

using BaseRecord = EpicsRecord<aiRecord>;

int32_t AiRecord::value() const {
    return this->raw()->rval;
}

void AiRecord::set_value(int32_t value) {
    this->raw()->rval = value;
}

static long record_ai_init(aiRecord *raw) {
    auto record = std::make_unique<AiRecord>(raw);
    BaseRecord::set_private_data(raw, std::move(record));
    framework_record_init(*BaseRecord::get_private_data(raw));
    return 0;
}

static long record_ai_get_ioint_info(int cmd, aiRecord *raw, IOSCANPVT *ppvt) {
    auto *record = BaseRecord::get_private_data(raw);
    ScanList scan_list;
    *ppvt = scan_list.raw();
    record->set_scan_list(std::move(scan_list));
    return 0;
}

static long record_ai_read(aiRecord *raw) {
    BaseRecord::get_private_data(raw)->process();
    return 0;
}

static long record_ai_linconv(aiRecord *raw, int after) {
    return 0;
}

struct AiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
    DEVSUPFUN special_linconv;
};

struct AiRecordCallbacks ai_record_handler = {
    6,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_ai_init),
    reinterpret_cast<DEVSUPFUN>(record_ai_get_ioint_info),
    reinterpret_cast<DEVSUPFUN>(record_ai_read),
    reinterpret_cast<DEVSUPFUN>(record_ai_linconv),
};

epicsExportAddress(dset, ai_record_handler);
