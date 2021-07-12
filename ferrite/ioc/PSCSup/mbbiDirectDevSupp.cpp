#include <cstdlib>
#include <iostream>

#include <pthread.h>

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
#include <mbbiDirectRecord.h>

#include "record/iointr.hpp" 
#include "record/mbbIODirect.hpp"

#include "iointrScanWorkers.hpp" 

static long mbbiDirect_init_record(mbbiDirectRecord *record_pointer);
static long mbbiDirect_get_iointr_info(
    int cmd,
    mbbiDirectRecord *record_pointer,
    IOSCANPVT *scan
);
static long mbbiDirect_record_read(mbbiDirectRecord *record_pointer);

static void mbbiDirect_record_read_callback(CALLBACK *callback_pointer);

struct mbbiDirectDevSuppSet {
    long num;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbiDirect;
};

struct mbbiDirectDevSuppSet mbbiDirect_device_support_handler = {
    5,
    NULL,
    NULL,
    reinterpret_cast<DEVSUPFUN>(mbbiDirect_init_record),
    reinterpret_cast<DEVSUPFUN>(mbbiDirect_get_iointr_info),
    reinterpret_cast<DEVSUPFUN>(mbbiDirect_record_read)
};

epicsExportAddress(dset, mbbiDirect_device_support_handler);


static long mbbiDirect_init_record(mbbiDirectRecord *record_pointer) {
    MbbiDirect mbbi_direct_record(record_pointer);
#ifdef RECORD_DEBUG
    std::cout << mbbi_direct_record.name() << " mbbiDirect_init_record()" << 
    std::endl << std::flush;
#endif

    mbbi_direct_record.set_callback(mbbiDirect_record_read_callback);

    iointr::init_iointr_scan_lists();

    return 0;
}

static long mbbiDirect_get_iointr_info(
    int cmd,
    mbbiDirectRecord *record_pointer,
    IOSCANPVT *scan
) {
    MbbiDirect mbbi_direct_record(record_pointer);

#ifdef RECORD_DEBUG
    std::cout << mbbi_direct_record.name() <<
    " mbbiDirect_get_iointr_info(), command = " <<
    cmd << std::endl << std::flush;
#endif

    iointr::init_scan_list(scan_list_name);
    *scan = iointr::get_scan_list(scan_list_name);
    
    return 0;
}

static long mbbiDirect_record_read(mbbiDirectRecord *record_pointer) {
    MbbiDirect mbbi_direct_record(record_pointer);
#ifdef RECORD_DEBUG
    std::cout << mbbi_direct_record.name() << 
    " mbbiDirect_record_read() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    if (mbbi_direct_record.pact() != true) {
        mbbi_direct_record.set_pact(true);
        mbbi_direct_record.request_callback();
    }
    
    return 0;
}

static void mbbiDirect_record_read_callback(CALLBACK *callback_struct_pointer) {
    struct dbCommon *record_pointer;
    // Line below doesn't work, because in C++ cast result is not lvalue
    // callbackGetUser((void *)(record_pointer), callback_struct_pointer);
    record_pointer = (dbCommon *)callback_struct_pointer->user;
    MbbiDirect mbbi_direct_record((mbbiDirectRecord *)record_pointer);

#ifdef RECORD_DEBUG
    std::cout << mbbi_direct_record.name() << 
    " mbbiDirect_record_read_callback() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    mbbi_direct_record.scan_lock();
    mbbi_direct_record.read();
    mbbi_direct_record.process_record();
    mbbi_direct_record.scan_unlock();
}