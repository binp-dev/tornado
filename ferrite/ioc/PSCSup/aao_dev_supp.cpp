#include <cstdlib>
#include <iostream>

#include <stdio.h>

#include <dbAccess.h>
#include <cantProceed.h>
#include <menuFtype.h>
#include <dbFldTypes.h>
#include <epicsTypes.h>
#include <aaoRecord.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsTypes.h>
#include <epicsExport.h>
#include <recSup.h>
#include <callback.h>


static long aao_record_init(aaoRecord *record_pointer);
static long aao_record_write(aaoRecord *record_pointer);
static void aao_record_write_callback(CALLBACK *callback_pointer);

struct aaoDevSuppSet {
    long num;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_aao;
};

struct aaoDevSuppSet aao_device_support_handler = {
    5,
    NULL,
    NULL,
    reinterpret_cast<DEVSUPFUN>(aao_record_init),
    NULL,
    reinterpret_cast<DEVSUPFUN>(aao_record_write)
};

epicsExportAddress(dset, aao_device_support_handler);

static long aao_record_init(aaoRecord *record_pointer) {
    std::cout << "  aao_record_init()" << std::endl << std::flush;

    record_pointer->bptr = callocMustSucceed(record_pointer->nelm, dbValueSize(record_pointer->ftvl), "calloc fail");
    epicsInt32 *pv_data = (epicsInt32 *)record_pointer->bptr; 
    record_pointer->nord = record_pointer->nelm;

    CALLBACK *callback_pointer;
    callback_pointer = (CALLBACK *)(calloc(1, sizeof(CALLBACK)));
    record_pointer->dpvt = (void *)callback_pointer; 
    callbackSetCallback(aao_record_write_callback, callback_pointer);
    callbackSetUser(record_pointer, callback_pointer);
    callbackSetPriority(priorityLow, callback_pointer);

    record_pointer->dpvt = (void *)callback_pointer;

    return 0;
}

static long aao_record_write(aaoRecord *record_pointer) {
    std::cout << "  aao_record_write()" << std::endl << std::flush;
    fflush(stdout);

    CALLBACK *callback_pointer = (CALLBACK *)record_pointer->dpvt;
    if (!record_pointer->pact) {
        record_pointer->pact = TRUE;
        callbackRequest(callback_pointer);
    }
    
    record_pointer->nord = record_pointer->nelm;
    
    return 0;
}

static void aao_record_write_callback(CALLBACK *callback_pointer) {
    std::cout << "  aao_record_write_callback()" << std::endl << std::flush;
    fflush(stdout);

    struct dbCommon *record_pointer;
    // callbackGetUser(record_pointer, callback_pointer);
    record_pointer = (dbCommon *)callback_pointer->user;
    fflush(stdout);

    struct typed_rset *rset_pointer = (struct typed_rset *)(record_pointer->rset);

    dbScanLock(record_pointer);
    (*rset_pointer->process)(record_pointer);

    aaoRecord *aao_pointer = (aaoRecord *)record_pointer;
    for (int i = 0; i < aao_pointer->nelm; ++i) {
        std::cout << ((epicsInt32 *)(aao_pointer->bptr))[i] << std::endl << std::flush;
    }
    dbScanUnlock(record_pointer);
}