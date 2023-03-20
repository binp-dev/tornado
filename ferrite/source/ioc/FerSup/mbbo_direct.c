#include <stdlib.h>

#include <devSup.h>
#include <epicsExport.h>
#include <mbboDirectRecord.h>
#include <recGbl.h>

#include "_record.h"

static long init(mbboDirectRecord *rec) {
    FerEpicsVar *var_info = (FerEpicsVar *)malloc(sizeof(FerEpicsVar));
    var_info->type = (FerVarType){
        FER_VAR_KIND_SCALAR,
        FER_VAR_DIR_READ,
        FER_VAR_SCALAR_TYPE_U32,
        1,
    };
    var_info->data = (void *)(&rec->rval);

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_MBBO_DIRECT, var_info);
    return 0;
}

static long get_ioint_info(int cmd, mbboDirectRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long write(mbboDirectRecord *rec) {
    fer_epics_record_process((dbCommon *)rec);
    return 0;
}

struct MbboDirectRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write;
};

struct MbboDirectRecordCallbacks mbbo_direct_record_handler = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)write,
};

epicsExportAddress(dset, mbbo_direct_record_handler);
