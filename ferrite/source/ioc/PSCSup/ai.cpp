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

static long record_ai_init(aiRecord *raw) {
    auto record = std::make_unique<AiRecord>(raw);
    EpicsRecord::set_private_data((dbCommon *)raw, std::move(record));
    framework_record_init(*EpicsRecord::get_private_data((dbCommon *)raw));
    return 0;
}

static long record_ai_get_ioint_info(int cmd, aiRecord *raw, IOSCANPVT *ppvt) {
    auto *record = EpicsRecord::get_private_data((dbCommon *)raw);
    ScanList scan_list;
    *ppvt = scan_list.raw();
    record->set_scan_list(std::move(scan_list));
    return 0;
}

static long record_ai_read(aiRecord *raw) {
    EpicsRecord::get_private_data((dbCommon *)raw)->process();
    return 0;
}

static long record_ai_linconv(aiRecord *raw, int after) {
    return 0;
}

struct AaiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
    DEVSUPFUN special_linconv;
};

struct AaiRecordCallbacks ai_record_handler = {
    6,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_ai_init),
    reinterpret_cast<DEVSUPFUN>(record_ai_get_ioint_info),
    reinterpret_cast<DEVSUPFUN>(record_ai_read),
    reinterpret_cast<DEVSUPFUN>(record_ai_linconv)
};

epicsExportAddress(dset, ai_record_handler);
