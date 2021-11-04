#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <aaoRecord.h>

#include "aao.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

std::unique_ptr<EpicsRecord> create_record(aaoRecord *raw) {
    return std::visit([&](auto phantom) -> std::unique_ptr<EpicsRecord> {
        using FinalRecord = AaoRecord<typename std::remove_reference_t<decltype(phantom)>::Type>;
        return std::make_unique<FinalRecord>(raw);
    }, epics_enum_type_variant(static_cast<menuFtype>(raw->ftvl)));
}

static long record_aao_init(aaoRecord *raw) {
    auto record = create_record(raw);
    EpicsRecord::set_private_data((dbCommon *)raw, std::move(record));
    framework_record_init(*EpicsRecord::get_private_data((dbCommon *)raw));
    return 0;
}

static long record_aao_write(aaoRecord *raw) {
    EpicsRecord::get_private_data((dbCommon *)raw)->process();
    return 0;
}

struct AaoRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_aao;
};

struct AaoRecordCallbacks aao_record_handler = {
    5,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_aao_init),
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_aao_write)
};

epicsExportAddress(dset, aao_record_handler);
