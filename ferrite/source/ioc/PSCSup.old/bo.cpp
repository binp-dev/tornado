#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <boRecord.h>

#include "bo.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

using BaseRecord = EpicsRecord<boRecord>;

bool BoRecord::value() const {
    return this->raw()->rval != 0;
}

static long record_bo_init(boRecord *raw) {
    auto record = std::make_unique<BoRecord>(raw);
    BaseRecord::set_private_data(raw, std::move(record));
    framework_record_init(*BaseRecord::get_private_data(raw));
    return 0;
}

static long record_bo_write(boRecord *raw) {
    BaseRecord::get_private_data(raw)->process();
    return 0;
}

static long record_bo_linconv(boRecord *raw, int after) {
    return 0;
}

struct BoRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_bo;
    DEVSUPFUN special_linconv;
};

struct BoRecordCallbacks bo_record_handler = {
    6,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_bo_init),
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_bo_write),
    reinterpret_cast<DEVSUPFUN>(record_bo_linconv),
};

epicsExportAddress(dset, bo_record_handler);
