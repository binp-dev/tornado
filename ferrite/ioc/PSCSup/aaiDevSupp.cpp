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
#include <aaiRecord.h>

#include "record/iointr.hpp" 
#include "record/analogArrayIO.hpp"

#include "iointrScanWorkers.hpp" 


static long aai_init_record(aaiRecord *record_pointer);
static long aai_get_iointr_info(
    int cmd,
    aaiRecord *record_pointer,
    IOSCANPVT *scan
);
static long aai_record_read(aaiRecord *record_pointer);

static void aai_record_read_callback(CALLBACK *callback_pointer);
// static void start_iointr_worker_thread(initHookState state);

struct AaiDevSuppSet {
    long num;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_iointr_info;
    DEVSUPFUN read_aai;
};

static struct AaiDevSuppSet aai_device_support_handler = {
    5,
    NULL,
    NULL,
    reinterpret_cast<DEVSUPFUN>(aai_init_record),
    reinterpret_cast<DEVSUPFUN>(aai_get_iointr_info),
    reinterpret_cast<DEVSUPFUN>(aai_record_read)
};

epicsExportAddress(dset, aai_device_support_handler);


static long aai_init_record(aaiRecord *record_pointer) {
    AnalogArrayInput aaiRec(record_pointer);
#ifdef RECORD_DEBUG
    std::cout << aaiRec.name() << " aai_init_record()" << std::endl << std::flush;
#endif

    aaiRec.set_callback(aai_record_read_callback);
    // initHookRegister(&start_iointr_worker_thread);

    return 0;
}

static long aai_get_iointr_info(
    int cmd,
    aaiRecord *record_pointer,
    IOSCANPVT *scan
) {
    AnalogArrayInput aaiRec(record_pointer);

#ifdef RECORD_DEBUG
    std::cout << aaiRec.name() << " aai_get_iointr_info(), command = "
    << cmd << std::endl << std::flush;
#endif

    iointr::init_scan_list(scan_list_name);
    *scan = iointr::get_scan_list(scan_list_name);
    
    return 0;
}

static long aai_record_read(aaiRecord *record_pointer) {
    AnalogArrayInput aaiRec(record_pointer);
#ifdef RECORD_DEBUG
    std::cout << aaiRec.name() << " aai_record_read() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    if (aaiRec.pact() != true) {
        aaiRec.set_pact(true);
        aaiRec.request_callback();
    }
    
    return 0;
}

static void aai_record_read_callback(CALLBACK *callback_struct_pointer) {
    struct dbCommon *record_pointer;
    // Line below doesn't work, because in C++ cast result is not lvalue
    // callbackGetUser((void *)(record_pointer), callback_struct_pointer);
    record_pointer = (dbCommon *)callback_struct_pointer->user;
    AnalogArrayInput aaiRec((aaiRecord *)record_pointer);

#ifdef RECORD_DEBUG
    std::cout << aaiRec.name() << " aai_record_read_callback() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    aaiRec.scan_lock();
    aaiRec.read();
    aaiRec.process_record();
    aaiRec.scan_unlock();
}

// static void start_iointr_worker_thread(initHookState state) {
//     if (state == initHookAfterInterruptAccept) {
// #ifdef RECORD_DEBUG
//     std::cout << "aaiDevSupp::start_iointr_worker_thread(), " << 
//     "EPICS hook state = initHookAfterInterruptAccept" << std::endl
//     << std::flush;
// #endif
//         iointr::start_scan_list_worker_thread(scan_list_name);
//     }
// }
