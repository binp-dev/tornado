#include <stdlib.h>

#include <biRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <recGbl.h>

#include "_record.h"

static long init(biRecord *rec) {
    FerEpicsVar *var_info = (FerEpicsVar *)malloc(sizeof(FerEpicsVar));
    var_info->type = (FerVarType){
        FER_VAR_KIND_SCALAR,
        FER_VAR_DIR_WRITE,
        FER_VAR_SCALAR_TYPE_U32,
        1,
    };
    var_info->data = (void *)(&rec->rval);

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_BI, var_info);
    return 0;
}

static long get_ioint_info(int cmd, biRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long read(biRecord *rec) {
    fer_epics_record_process((dbCommon *)rec);
    return 0;
}

struct BiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
};

struct BiRecordCallbacks bi_record_handler = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read,
};

epicsExportAddress(dset, bi_record_handler);
