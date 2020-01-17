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

#include "framework.hpp"


void init(void) {
    printf("init\n");
}

long record_waveform_init(waveformRecord *record) {
    std::cout << "record_waveform_init: " << record->name << std::endl;
    std::unique_ptr<GenericWaveformRecord> user_record;

    try {
        user_record = framework_init_record<GenericWaveformRecord>(record->name);
    } catch (const std::exception &e) {
        std::cerr << "Exception caught" << std::endl;
        std::cerr << e.what() << std::endl;
        epicsExit(1);
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        epicsExit(1);
    }

    assert(record->ftvl == user_record->generic_type());
    record->dpvt = reinterpret_cast<void*>(user_record.release());
    return 0;
}
long record_waveform_get_ioint_info(int cmd, waveformRecord *record, IOSCANPVT *ppvt) {
    std::cout << "record_waveform_get_ioint_info: " << record->name << std::endl;
    std::cerr << "unimplemented" << std::endl;
    return 0;
}
long record_waveform_read(waveformRecord *record) {
    std::cout << "record_waveform_read: " << record->name << std::endl;
    GenericWaveformRecord *user_record = reinterpret_cast<GenericWaveformRecord*>(record->dpvt);

    try {
        user_record->read_generic(record->bptr, record->nord, record->ftvl);
    } catch (const std::exception &e) {
        std::cerr << "Exception caught" << std::endl;
        std::cerr << e.what() << std::endl;
        epicsExit(1);
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        epicsExit(1);
    }

    return 0;
}

struct WaveformRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_waveform;
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

epicsExportRegistrar(init);
