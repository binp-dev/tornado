#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <waveformRecord.h>

#include "waveform.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

std::unique_ptr<EpicsRecord> create_record(waveformRecord *raw) {
    return std::visit([&](auto phantom) -> std::unique_ptr<EpicsRecord> {
        using FinalRecord = WaveformRecord<typename std::remove_reference_t<decltype(phantom)>::Type>;
        return std::make_unique<FinalRecord>(raw);
    }, epics_enum_type_variant(static_cast<menuFtype>(raw->ftvl)));
}

static long record_waveform_init(waveformRecord *raw) {
    auto record = create_record(raw);
    EpicsRecord::set_private_data((dbCommon *)raw, std::move(record));
    framework_record_init(*EpicsRecord::get_private_data((dbCommon *)raw));
    return 0;
}

static long record_waveform_get_ioint_info(int cmd, waveformRecord *raw, IOSCANPVT *ppvt) {
    unimplemented();
}

static long record_waveform_read(waveformRecord *raw) {
    EpicsRecord::get_private_data((dbCommon *)raw)->process();
    return 0;
}

struct WaveformRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_wf;
};

struct WaveformRecordCallbacks waveform_record_handler = {
    5,
    nullptr,
    nullptr,
    reinterpret_cast<DEVSUPFUN>(record_waveform_init),
    reinterpret_cast<DEVSUPFUN>(record_waveform_get_ioint_info),
    reinterpret_cast<DEVSUPFUN>(record_waveform_read)
};

epicsExportAddress(dset, waveform_record_handler);
