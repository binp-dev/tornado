#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <aoRecord.h>

#include "ao.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

int32_t AoRecord::value() const {
    return this->raw()->rval;
}

static long record_ao_init(aoRecord *raw) {
    auto record = std::make_unique<AoRecord>(raw);
    EpicsRecord::set_private_data((dbCommon *)raw, std::move(record));
    framework_record_init(*EpicsRecord::get_private_data((dbCommon *)raw));
    return 0;
}

static long record_ao_write(aoRecord *raw) {
    EpicsRecord::get_private_data((dbCommon *)raw)->process();
    return 0;
}

static long record_ao_linconv(aoRecord *raw, int after) {
    return 0;
}

struct AoRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_ao;
    DEVSUPFUN special_linconv;
};

struct AoRecordCallbacks ao_record_handler = {
    6,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_ao_init),
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_ao_write),
    reinterpret_cast<DEVSUPFUN>(record_ao_linconv)
};

epicsExportAddress(dset, ao_record_handler);
