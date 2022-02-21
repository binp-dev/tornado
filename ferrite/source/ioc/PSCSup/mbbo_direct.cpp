#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <mbboDirectRecord.h>

#include "mbbo_direct.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

uint32_t MbboDirectRecord::value() const {
    return this->raw()->rval;
}

static long record_mbbo_direct_init(mbboDirectRecord *raw) {
    auto record = std::make_unique<MbboDirectRecord>(raw);
    EpicsRecord::set_private_data((dbCommon *)raw, std::move(record));
    framework_record_init(*EpicsRecord::get_private_data((dbCommon *)raw));
    return 0;
}

static long record_mbbo_direct_write(mbboDirectRecord *raw) {
    EpicsRecord::get_private_data((dbCommon *)raw)->process();
    return 0;
}

struct AaiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbbo_direct;
};

struct AaiRecordCallbacks mbbo_direct_record_handler = {
    5,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_mbbo_direct_init),
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_mbbo_direct_write)
};

epicsExportAddress(dset, mbbo_direct_record_handler);
