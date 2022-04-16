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

using BaseRecord = EpicsRecord<mbbiDirectRecord>;

uint32_t MbbiDirectRecord::value() const {
    return this->raw()->rval;
}

void MbbiDirectRecord::set_value(uint32_t value) {
    this->raw()->rval = value;
}

static long record_mbbi_direct_init(mbbiDirectRecord *raw) {
    auto record = std::make_unique<MbbiDirectRecord>(raw);
    BaseRecord::set_private_data(raw, std::move(record));
    framework_record_init(*BaseRecord::get_private_data(raw));
    return 0;
}

static long record_mbbi_direct_get_ioint_info(int cmd, mbbiDirectRecord *raw, IOSCANPVT *ppvt) {
    auto *record = BaseRecord::get_private_data(raw);
    ScanList scan_list;
    *ppvt = scan_list.raw();
    record->set_scan_list(std::move(scan_list));
    return 0;
}

static long record_mbbi_direct_read(mbbiDirectRecord *raw) {
    BaseRecord::get_private_data(raw)->process();
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
    reinterpret_cast<DEVSUPFUN>(record_mbbi_direct_read),
};

epicsExportAddress(dset, mbbi_direct_record_handler);
