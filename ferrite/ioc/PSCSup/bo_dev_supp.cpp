#include <cstdlib>
#include <iostream>

#include <stdio.h>

#include <dbAccess.h>
#include <cantProceed.h>
#include <menuFtype.h>
#include <dbFldTypes.h>
#include <epicsTypes.h>
#include <boRecord.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsTypes.h>
#include <epicsExport.h>
#include <recSup.h>
#include <callback.h>


static long bo_record_init(boRecord *record_pointer);
static long bo_record_write(boRecord *record_pointer);
static void bo_record_write_callback(CALLBACK *callback_pointer);

struct boDevSuppSet {
    long num;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_bo;
};

struct boDevSuppSet bo_device_support_handler = {
    5,
    NULL,
    NULL,
    reinterpret_cast<DEVSUPFUN>(bo_record_init),
    NULL,
    reinterpret_cast<DEVSUPFUN>(bo_record_write)
};

epicsExportAddress(dset, bo_device_support_handler);

static long bo_record_init(boRecord *record_pointer) {
    std::cout << "  bo_record_init()" << std::endl << std::flush;

    CALLBACK *callback_pointer;
    callback_pointer = (CALLBACK *)(calloc(1, sizeof(CALLBACK)));
    record_pointer->dpvt = (void *)callback_pointer; 
    callbackSetCallback(bo_record_write_callback, callback_pointer);
    callbackSetUser(record_pointer, callback_pointer);
    callbackSetPriority(priorityLow, callback_pointer);

    record_pointer->dpvt = (void *)callback_pointer;

    return 0;
}

static long bo_record_write(boRecord *record_pointer) {
    std::cout << "  bo_record_write()" << std::endl << std::flush;

    CALLBACK *callback_pointer = (CALLBACK *)record_pointer->dpvt;
    if (!record_pointer->pact) {
        record_pointer->pact = TRUE;
        callbackRequest(callback_pointer);
    }
    
    return 0;
}

static void bo_record_write_callback(CALLBACK *callback_pointer) {
    std::cout << "  bo_record_write_callback()" << std::endl << std::flush;

    struct dbCommon *record_pointer;
    // callbackGetUser(record_pointer, callback_pointer);
    record_pointer = (dbCommon *)callback_pointer->user;
    struct typed_rset *rset_pointer = (struct typed_rset *)(record_pointer->rset);

    dbScanLock(record_pointer);
    (*rset_pointer->process)(record_pointer);
    dbScanUnlock(record_pointer);
}