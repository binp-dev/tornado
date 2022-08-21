#include <stdio.h>
#include <stdlib.h>

#include <aaoRecord.h>
#include <devSup.h>
#include <epicsExport.h>
#include <recGbl.h>

#include "_array_record.h"
#include "_record.h"

static long init(aaoRecord *rec) {
    FerEpicsVarArray *var_info = (FerEpicsVarArray *)malloc(sizeof(FerEpicsVarArray));
    var_info->base.type = (FerVarType){
        FER_VAR_KIND_ARRAY,
        FER_VAR_DIR_READ,
        fer_epics_convert_scalar_type((menuFtype)rec->ftvl),
        rec->nelm,
    };
    var_info->base.data = (void *)rec->bptr;
    printf("@ aao->bptr = 0x%p\n", rec->bptr);
    var_info->len_ptr = &rec->nord;

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_AAO, (FerEpicsVar *)var_info);
    return 0;
}

static long get_ioint_info(int cmd, aaoRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long write(aaoRecord *rec) {
    printf("@ aao->bptr = 0x%p\n", rec->bptr);
    fer_epics_record_process((dbCommon *)rec);
    return 0;
}

struct AaoRecordCallbacks {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write;
};

struct AaoRecordCallbacks aao_record_handler = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)write,
};

epicsExportAddress(dset, aao_record_handler);
