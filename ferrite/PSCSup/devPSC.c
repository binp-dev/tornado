#include <stdio.h>
#include <stdlib.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>

#include <waveformRecord.h>

#include <epicsExport.h>

#include <iocsh.h>

void print_data(struct waveformRecord *record) {
    printf("nelm: %d\n", record->nelm);
    printf("nord: %d\n", record->nord);
    /*
    printf("bptr: [");
    int i = 0;
    for (i = 0; i < record->nord; ++i) {
        printf(" %.2lf, ", ((double*)record->bptr)[i]);
    }
    printf(" ]\n");
    */
}

void init(void) {
    printf("init\n");
}

long init_record     (struct waveformRecord *record) {
    printf("init_record: %s\n", record->name);
    print_data(record);
    return 0;
}
long get_ioint_info  (int cmd, struct waveformRecord *record, IOSCANPVT *ppvt) {
    printf("get_ioint_info: %s\n", record->name);
    return 0;
}
long read_waveform   (struct waveformRecord *record) {
    printf("read_waveform: %s\n", record->name);
    print_data(record);
    return 0;
}

struct Waveform {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_waveform;
};

struct Waveform rec_waveform = {
    5,
    NULL,
    NULL,
    init_record,
    get_ioint_info,
    read_waveform
};

epicsExportAddress(dset, rec_waveform);

epicsExportRegistrar(init);
