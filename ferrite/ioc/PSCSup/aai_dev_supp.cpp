#include <cstdlib>
#include <iostream>

#include <dbAccess.h>
#include <cantProceed.h>
#include <menuFtype.h>
#include <dbFldTypes.h>
#include <epicsTypes.h>
#include <aaiRecord.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsTypes.h>
#include <epicsExport.h>
#include <recSup.h>
#include <callback.h>
#include <initHooks.h>

#include <pthread.h>

#include "record/ioscan.hpp" 
#include "record/analogArrayIO.hpp"

static long aai_init(int phase);
static long aai_init_record(aaiRecord *record_pointer);
static long aai_get_iointr_info(int cmd, aaiRecord *record_pointer, IOSCANPVT *scan);
static long aai_record_read(aaiRecord *record_pointer);

static void aai_record_read_callback(CALLBACK *callback_pointer);
static void start_workers_hook(initHookState state);
static void ioscan_worker(void *args);

std::string ioscan_list_name = "TEST SCAN LIST";

struct AaiDevSuppSet {
    long num;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_iointr_info;
    DEVSUPFUN read_aai;
};

struct AaiDevSuppSet aai_device_support_handler = {
    5,
    NULL,
    reinterpret_cast<DEVSUPFUN>(aai_init),
    reinterpret_cast<DEVSUPFUN>(aai_init_record),
    reinterpret_cast<DEVSUPFUN>(aai_get_iointr_info),
    reinterpret_cast<DEVSUPFUN>(aai_record_read)
};

epicsExportAddress(dset, aai_device_support_handler);

static long aai_init(int phase) {
    std::cout << "  aai_init(), phase = " << phase << std::endl << std::flush;

    if (phase == 0) {
        initHookRegister(&start_workers_hook);
    }

    return 0;
}

static long aai_init_record(aaiRecord *record_pointer) {
    std::cout << "  aai_init_record()" << std::endl << std::flush;
    AnalogArrayInput aaiRec(record_pointer);
    aaiRec.set_callback(aai_record_read_callback);
    
    return 0;
}

static long aai_get_iointr_info(
    int cmd,
    aaiRecord *record_pointer,
    IOSCANPVT *scan
) {
    std::cout << "  aai_get_iointr_info(), cmd = " <<cmd << std::endl 
    << std::flush;

    AnalogArrayInput aaiRec(record_pointer);
    IOScan::init_ioscan_list(
        ioscan_list_name,
        &ioscan_worker,
        aaiRec.get_private_data()
    );
    *scan = IOScan::get_ioscan_list(ioscan_list_name);
}

static long aai_record_read(aaiRecord *record_pointer) {
    std::cout << "  aai_record_read()" << std::endl << std::flush;
    std::cout << "      Thread id = " << pthread_self() << std::endl << std::flush;

    AnalogArrayInput aaiRec(record_pointer);
    if (aaiRec.get_pact() != true) {
        aaiRec.set_pact(true);
        aaiRec.request_callback();
    }
    
    record_pointer->nord = record_pointer->nelm;

    return 0;
}

static void aai_record_read_callback(CALLBACK *callback_struct_pointer) {
    std::cout << "  aai_record_read_callback()" << std::endl << std::flush;
    std::cout << "      Thread id = " << pthread_self() << std::endl << std::flush;


    struct dbCommon *record_pointer;
    // Line below not working, because in C++ cast result is not lvalue
    // callbackGetUser((void *)(record_pointer), callback_struct_pointer);
    record_pointer = (dbCommon *)callback_struct_pointer->user;

    AnalogArrayInput aaiRec((aaiRecord *)record_pointer);
    aaiRec.scan_lock();
    aaiRec.read();
    aaiRec.process_record();
    aaiRec.scan_unlock();
}

static void start_workers_hook(initHookState state) {
    std::cout << "  start_workers_hook()" << std::endl << std::flush;

    if (state == initHookAfterInterruptAccept) {
        IOScan::start_ioscan_list_worker(ioscan_list_name);
        std::cout << "      hook state = initHookAfterInterruptAccept" << std::endl << std::flush;
    }
}

static void ioscan_worker(void *args) {
    std::cout << "  IOSCAN WORKER THREAD START" << std::endl << std::flush;
    std::cout << "      Thread id = " << pthread_self() << std::endl << std::flush;
    IOSCANPVT &scan = IOScan::get_ioscan_list(ioscan_list_name);

    while (true) {
        scanIoImmediate(scan, priorityLow);
        scanIoImmediate(scan, priorityHigh);
        scanIoImmediate(scan, priorityMedium);
        scanIoRequest(scan);
        
        epicsThreadSleep(5.0);
    }
}