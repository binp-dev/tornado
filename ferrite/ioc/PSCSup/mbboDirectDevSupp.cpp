#include <iostream>

#include <dbAccess.h>
#include <cantProceed.h>
#include <menuFtype.h>
#include <dbFldTypes.h>
#include <epicsTypes.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsTypes.h>
#include <epicsExport.h>
#include <recSup.h>
#include <callback.h>
#include <initHooks.h>
#include <mbboDirectRecord.h>

#include <pthread.h>

#include "record/mbbIODirect.hpp"


static long mbboDirect_init_record(mbboDirectRecord *record_pointer);
static long mbboDirect_record_write(mbboDirectRecord *record_pointer);

static void mbboDirect_record_write_callback(CALLBACK *callback_pointer);

struct mbboDirectDevSuppSet {
    long num;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_iointr_info;
    DEVSUPFUN write_mbboDirect;
};

static struct mbboDirectDevSuppSet mbboDirect_device_support_handler = {
    5,
    NULL,
    NULL,
    reinterpret_cast<DEVSUPFUN>(mbboDirect_init_record),
    NULL,
    reinterpret_cast<DEVSUPFUN>(mbboDirect_record_write)
};

epicsExportAddress(dset, mbboDirect_device_support_handler);


static long mbboDirect_init_record(mbboDirectRecord *record_pointer) {
    MbboDirect mbbo_direct_record(record_pointer);
#ifdef RECORD_DEBUG
    std::cout << mbbo_direct_record.name() << " mbboDirect_init_record()" << std::endl << std::flush;
#endif
    mbbo_direct_record.set_callback(mbboDirect_record_write_callback);

    return 0;
}

static long mbboDirect_record_write(mbboDirectRecord *record_pointer) {
    MbboDirect mbbo_direct_record(record_pointer);
#ifdef RECORD_DEBUG
    std::cout << mbbo_direct_record.name() << " mbboDirect_record_write() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    if (mbbo_direct_record.pact() != true) {
        mbbo_direct_record.set_pact(true);
        mbbo_direct_record.request_callback();
    }
    
    return 0;
}

static void mbboDirect_record_write_callback(CALLBACK *callback_struct_pointer) {
    struct dbCommon *record_pointer;
    // Line below doesn't work, because in C++ cast result is not lvalue
    // callbackGetUser((void *)(record_pointer), callback_struct_pointer);
    record_pointer = (dbCommon *)callback_struct_pointer->user;

    MbboDirect mbbo_direct_record((mbboDirectRecord *)record_pointer);
#ifdef RECORD_DEBUG
    std::cout << mbbo_direct_record.name() << " mbboDirect_record_write_callback() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    mbbo_direct_record.scan_lock();
    mbbo_direct_record.write();
    mbbo_direct_record.process_record();
    mbbo_direct_record.scan_unlock();
}