#include <stdlib.h>

#include <aiRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <recGbl.h>

#include "_record.h"

static long init(aiRecord *rec) {
    FerEpicsVar *var_info = (FerEpicsVar *)malloc(sizeof(FerEpicsVar));
    var_info->type = (FerVarType){
        FER_VAR_KIND_SCALAR,
        FER_VAR_DIR_WRITE,
        FER_VAR_SCALAR_TYPE_I32,
        1,
    };
    var_info->data = (void *)(&rec->rval);

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_AI, var_info);
    return 0;
}

static long get_ioint_info(int cmd, aiRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long read(aiRecord *rec) {
    fer_epics_record_process((dbCommon *)rec);
    return 0;
}

static long linconv(aiRecord *rec, int after) {
    return 0;
}

struct AiRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
    DEVSUPFUN special_linconv;
};

struct AiRecordCallbacks ai_record_handler = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read,
    (DEVSUPFUN)linconv,
};

epicsExportAddress(dset, ai_record_handler);
