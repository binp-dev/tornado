#include <stdlib.h>

#include <aoRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <recGbl.h>

#include "_record.h"

static long init(aoRecord *rec) {
    FerEpicsVar *var_info = (FerEpicsVar *)malloc(sizeof(FerEpicsVar));
    var_info->type = (FerVarType){
        FER_VAR_KIND_SCALAR,
        FER_VAR_DIR_READ,
        FER_VAR_SCALAR_TYPE_I32,
        1,
    };
    var_info->data = (void *)(&rec->rval);

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_AO, var_info);
    return 0;
}

static long get_ioint_info(int cmd, aoRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long write(aoRecord *rec) {
    fer_epics_record_process((dbCommon *)rec);

    return 0;
}

static long linconv(aoRecord *rec, int after) {
    return 0;
}

struct AoRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write;
    DEVSUPFUN special_linconv;
};

struct AoRecordCallbacks ao_record_handler = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)write,
    (DEVSUPFUN)linconv,
};

epicsExportAddress(dset, ao_record_handler);
