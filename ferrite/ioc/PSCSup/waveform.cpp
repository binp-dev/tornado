#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExit.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <waveformRecord.h>

#include "waveform.hpp"

#include <core/panic.hpp>
#include <framework.hpp>

template <typename T, typename Visitor>
decltype(auto) with_record(waveformRecord &raw, Visitor &&visitor) {
    WaveformRecord<T> record(raw);
    return visitor(record);
}

static long record_waveform_init(waveformRecord *raw) {
    return visit_epics_enum<with_record>(static_cast<menuFtype>(raw->ftvl), *raw, [&](auto &record) {
        auto handler = framework_record_init(record);
        assert_ne(handler, nullptr);
        record.set_handler(handler);
        return 0;
    });
}

static long record_waveform_get_ioint_info(int cmd, waveformRecord *raw, IOSCANPVT *ppvt) {
    unimplemented();
}

static long record_waveform_read(waveformRecord *raw) {
    return visit_epics_enum<with_record>(static_cast<menuFtype>(raw->ftvl), *raw, [&](auto &record) {
        record.process();
        return 0;
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
