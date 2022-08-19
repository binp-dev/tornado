#include <stdlib.h>

#include <aaiRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <recGbl.h>

#include "_array.h"
#include "_record.h"

static long record_aai_init(aaiRecord *rec) {
    FerEpicsVarArray *var_info = (FerEpicsVarArray *)malloc(sizeof(FerEpicsVarArray));
    var_info->base.type = (FerVarType){
        FER_VAR_KIND_ARRAY,
        FER_VAR_DIR_INPUT,
        fer_epics_convert_scalar_type((menuFtype)rec->ftvl),
        rec->nelm,
    };
    var_info->base.data = (void *)rec->bptr;
    var_info->len_ptr = &rec->nord;

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_AAI, (FerEpicsVar *)var_info);
    return 0;
}

static long record_aai_get_ioint_info(int cmd, aaiRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long record_aai_read(aaiRecord *rec) {
    fer_epics_record_process((dbCommon *)rec);
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
    NULL,
    NULL,
    (DEVSUPFUN)record_aai_init,
    (DEVSUPFUN)record_aai_get_ioint_info,
    (DEVSUPFUN)record_aai_read,
};

epicsExportAddress(dset, aai_record_handler);
