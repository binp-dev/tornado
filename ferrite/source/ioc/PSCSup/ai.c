#include <stdlib.h>

#include <devSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <aiRecord.h>

#include "_common.h"
#include "_interface.h"

static long record_ai_init(aiRecord *rec) {
    fer_epics_record_init((dbCommon *)rec);
    return 0;
}

static long record_ai_get_ioint_info(int cmd, aiRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long record_ai_read(aiRecord *rec) {
    fer_epics_record_process((dbCommon *)rec);
    return 0;
}

static long record_ai_linconv(aiRecord *rec, int after) {
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
    (DEVSUPFUN)record_ai_init,
    (DEVSUPFUN)record_ai_get_ioint_info,
    (DEVSUPFUN)record_ai_read,
    (DEVSUPFUN)record_ai_linconv,
};

epicsExportAddress(dset, ai_record_handler);
