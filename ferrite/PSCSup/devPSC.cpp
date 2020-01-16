#include <stdio.h>
#include <stdlib.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>

#include <waveformRecord.h>

#include <epicsExport.h>

#include <iocsh.h>

#include "framework.hpp"


extern "C" {
    void init(void);
    long record_waveform_init(waveformRecord *record);
    long record_waveform_get_ioint_info(int cmd, waveformRecord *record, IOSCANPVT *ppvt);
    long record_waveform_read(waveformRecord *record);
}

/*
void print_data(waveformRecord *record) {
    printf("nelm: %d\n", record->nelm);
    printf("nord: %d\n", record->nord);
    printf("bptr: [");
    int i = 0;
    for (i = 0; i < record->nord; ++i) {
        printf(" %.2lf, ", ((double*)record->bptr)[i]);
    }
    printf(" ]\n");
}
*/

void init(void) {
    printf("init\n");
}

long record_waveform_init(waveformRecord *record) {
    printf("record_waveform_init: %s\n", record->name);
    std::unique_ptr<GenericWaveformRecord> user_record = framework_init_record<GenericWaveformRecord>(record->name);
    assert(record->ftvl == user_record->generic_type());
    record->dpvt = reinterpret_cast<void*>(user_record.release());
    return 0;
}
long record_waveform_get_ioint_info(int cmd, waveformRecord *record, IOSCANPVT *ppvt) {
    printf("record_waveform_get_ioint_info: %s\n", record->name);
    printf("unimplemented\n");
    return 0;
}
long record_waveform_read(waveformRecord *record) {
    printf("record_waveform_read: %s\n", record->name);
    GenericWaveformRecord *user_record = reinterpret_cast<GenericWaveformRecord*>(record->dpvt);
    user_record->read_generic(record->bptr, record->nord, record->ftvl);
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
