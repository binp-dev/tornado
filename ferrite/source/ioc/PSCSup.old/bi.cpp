#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <biRecord.h>

#include "bi.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

using BaseRecord = EpicsRecord<biRecord>;

bool BiRecord::value() const {
    return this->raw()->rval != 0;
}

void BiRecord::set_value(bool value) {
    this->raw()->rval = uint32_t(value);
}

static long record_bi_init(biRecord *raw) {
    auto record = std::make_unique<BiRecord>(raw);
    BaseRecord::set_private_data(raw, std::move(record));
    framework_record_init(*BaseRecord::get_private_data(raw));
    return 0;
}

static long record_bi_get_ioint_info(int cmd, biRecord *raw, IOSCANPVT *ppvt) {
    auto *record = BaseRecord::get_private_data(raw);
    ScanList scan_list;
    *ppvt = scan_list.raw();
    record->set_scan_list(std::move(scan_list));
    return 0;
}

static long record_bi_read(biRecord *raw) {
    BaseRecord::get_private_data(raw)->process();
    return 0;
}

static long record_bi_linconv(biRecord *raw, int after) {
    return 0;
}

struct BiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_bi;
    DEVSUPFUN special_linconv;
};

struct BiRecordCallbacks bi_record_handler = {
    6,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_bi_init),
    reinterpret_cast<DEVSUPFUN>(record_bi_get_ioint_info),
    reinterpret_cast<DEVSUPFUN>(record_bi_read),
    reinterpret_cast<DEVSUPFUN>(record_bi_linconv),
};

epicsExportAddress(dset, bi_record_handler);
