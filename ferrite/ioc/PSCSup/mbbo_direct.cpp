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

static long record_mbbo_direct_init(mbboDirectRecord *raw) {
    auto record = std::make_unique<MbboDirectRecord>(raw);
    EpicsRecord::set_private_data((dbCommon *)raw, std::move(record));
    framework_record_init(*EpicsRecord::get_private_data((dbCommon *)raw));
    return 0;
}

static long record_mbbo_direct_get_ioint_info(int cmd, mbboDirectRecord *raw, IOSCANPVT *ppvt) {
    auto *record = EpicsRecord::get_private_data((dbCommon *)raw);
    ScanList scan_list;
    *ppvt = scan_list.raw();
    record->set_scan_list(std::move(scan_list));
    return 0;
}

static long record_mbbo_direct_read(mbboDirectRecord *raw) {
    EpicsRecord::get_private_data((dbCommon *)raw)->process();
    return 0;
}

struct AaiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbo_direct;
};

struct AaiRecordCallbacks mbbo_direct_record_handler = {
    5,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_mbbo_direct_init),
    reinterpret_cast<DEVSUPFUN>(record_mbbo_direct_get_ioint_info),
    reinterpret_cast<DEVSUPFUN>(record_mbbo_direct_read)
};

epicsExportAddress(dset, mbbo_direct_record_handler);
