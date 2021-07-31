#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <mbbiDirectRecord.h>

#include "mbbi_direct.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

static long record_mbbi_direct_init(mbbiDirectRecord *raw) {
    auto record = std::make_unique<MbbiDirectRecord>(raw);
    EpicsRecord::set_private_data((dbCommon *)raw, std::move(record));
    framework_record_init(*EpicsRecord::get_private_data((dbCommon *)raw));
    return 0;
}

static long record_mbbi_direct_get_ioint_info(int cmd, mbbiDirectRecord *raw, IOSCANPVT *ppvt) {
    unimplemented();
}

static long record_mbbi_direct_read(mbbiDirectRecord *raw) {
    EpicsRecord::get_private_data((dbCommon *)raw)->process();
    return 0;
}

struct AaiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbi_direct;
};

struct AaiRecordCallbacks mbbi_direct_record_handler = {
    5,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_mbbi_direct_init),
    reinterpret_cast<DEVSUPFUN>(record_mbbi_direct_get_ioint_info),
    reinterpret_cast<DEVSUPFUN>(record_mbbi_direct_read)
};

epicsExportAddress(dset, mbbi_direct_record_handler);
