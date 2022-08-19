#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <aaiRecord.h>

#include "aai.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

using BaseRecord = EpicsRecord<aaiRecord>;

std::unique_ptr<BaseRecord> create_record(aaiRecord *raw) {
    return std::visit(
        [&](auto phantom) -> std::unique_ptr<BaseRecord> {
            using FinalRecord = AaiRecord<typename std::remove_reference_t<decltype(phantom)>::Type>;
            return std::make_unique<FinalRecord>(raw);
        },
        epics_enum_type_variant(static_cast<menuFtype>(raw->ftvl)) //
    );
}

static long record_aai_init(aaiRecord *raw) {
    auto record = create_record(raw);
    BaseRecord::set_private_data(raw, std::move(record));
    framework_record_init(*BaseRecord::get_private_data(raw));
    return 0;
}

static long record_aai_get_ioint_info(int cmd, aaiRecord *raw, IOSCANPVT *ppvt) {
    auto *record = BaseRecord::get_private_data(raw);
    ScanList scan_list;
    *ppvt = scan_list.raw();
    record->set_scan_list(std::move(scan_list));
    return 0;
}

static long record_aai_read(aaiRecord *raw) {
    BaseRecord::get_private_data(raw)->process();
    return 0;
}

struct AaiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_aai;
};

struct AaiRecordCallbacks aai_record_handler = {
    5,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_aai_init),
    reinterpret_cast<DEVSUPFUN>(record_aai_get_ioint_info),
    reinterpret_cast<DEVSUPFUN>(record_aai_read),
};

epicsExportAddress(dset, aai_record_handler);
