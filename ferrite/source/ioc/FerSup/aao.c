#include <stdlib.h>
#include <string.h>

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
    var_info->item_size = fer_epics_scalar_type_size((menuFtype)rec->ftvl);
    // Create additional buffer to store copy of data.
    // See note in `write` function below.
    var_info->locked_data = malloc(rec->nelm * var_info->item_size);
    var_info->locked_len = 0;

    var_info->base.data = var_info->locked_data;
    var_info->len_ptr = &var_info->locked_len;

    fer_epics_record_init((dbCommon *)rec, FER_EPICS_RECORD_TYPE_AAO, (FerEpicsVar *)var_info);
    return 0;
}

static long get_ioint_info(int cmd, aaoRecord *rec, IOSCANPVT *ppvt) {
    *ppvt = fer_epics_record_ioscan_create((dbCommon *)rec);
    return 0;
}

static long write(aaoRecord *rec) {
    FerEpicsVarArray *var_info = (FerEpicsVarArray *)fer_epics_record_var_info((dbCommon *)rec);
    // `aaoRecord->(bptr/nord)` are updated on write even if record is processing (`PACT` is true).
    // To mitigate this issue we make a copy of data and length on processing start.
    memcpy(var_info->locked_data, rec->bptr, rec->nord * var_info->item_size);
    var_info->locked_len = rec->nord;

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
