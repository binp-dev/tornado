#include <stdlib.h>

#include <boRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <recGbl.h>

#include "_record.h"

static long init(boRecord *rec) {
    FerEpicsVar *var_info = (FerEpicsVar *)malloc(sizeof(FerEpicsVar));
    var_info->type = (FerVarType){
        FER_VAR_KIND_SCALAR,
        FER_VAR_DIR_READ,
        FER_VAR_SCALAR_TYPE_U32,
        1,
    };
    var_info->data = (void *)(&rec->rval);

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_BO, var_info);
    return 0;
}

static long get_ioint_info(int cmd, boRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long write(boRecord *rec) {
    fer_epics_record_process((dbCommon *)rec);
    return 0;
}

struct BoRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write;
};

struct BoRecordCallbacks bo_record_handler = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)write,
};

epicsExportAddress(dset, bo_record_handler);
