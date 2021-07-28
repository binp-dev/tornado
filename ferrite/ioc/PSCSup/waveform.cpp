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

template <typename Visitor>
void visit_record(waveformRecord *raw, Visitor &&visitor) {
    std::visit([&](const auto &phantom) {
        WaveformRecord<typename std::remove_reference_t<decltype(phantom)>::Type> record(raw);
        visitor(record);
    }, epics_enum_type_variant(static_cast<menuFtype>(raw->ftvl)));
}

long record_waveform_init(waveformRecord *raw) {
    visit_record(raw, [](auto &record) {
        record.initialize();
        framework_record_init(record);
    });
    return 0;
}

long record_waveform_get_ioint_info(int cmd, waveformRecord *raw, IOSCANPVT *ppvt) {
    unimplemented();
}

long record_waveform_read(waveformRecord *raw) {
    visit_record(raw, [](auto &record) {
        record.process();
    });
    return 0;
}

void waveform_async_process_callback(epicsCallback *callback) {
    visit_record((waveformRecord *)(callback->user), [](auto &record) {
        record.process_async();
    });
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
