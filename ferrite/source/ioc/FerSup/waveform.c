#include <stdlib.h>

#include <devSup.h>
#include <epicsExport.h>
#include <recGbl.h>
#include <waveformRecord.h>

#include "_array_record.h"
#include "_record.h"

static long init(waveformRecord *rec) {
    FerEpicsVarArray *var_info = (FerEpicsVarArray *)malloc(sizeof(FerEpicsVarArray));
    var_info->base.type = (FerVarType){
        FER_VAR_KIND_ARRAY,
        FER_VAR_DIR_WRITE,
        fer_epics_convert_scalar_type((menuFtype)rec->ftvl),
        rec->nelm,
    };
    var_info->item_size = fer_epics_scalar_type_size((menuFtype)rec->ftvl);
    var_info->base.data = NULL;
    var_info->len_ptr = &rec->nord;

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_WAVEFORM, (FerEpicsVar *)var_info);
    return 0;
}

static long get_ioint_info(int cmd, waveformRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long read(waveformRecord *rec) {
    FerEpicsVarArray *var_info = (FerEpicsVarArray *)fer_epics_record_var_info((dbCommon *)rec);
    var_info->base.data = (void *)rec->bptr;

    fer_epics_record_process((dbCommon *)rec);
    return 0;
}

struct WaveformRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
};

struct WaveformRecordCallbacks waveform_record_handler = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read,
};

epicsExportAddress(dset, waveform_record_handler);
