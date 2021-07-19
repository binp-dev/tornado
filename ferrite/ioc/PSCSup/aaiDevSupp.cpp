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


static long aai_init_record(aaiRecord *record_pointer);
static long aai_get_iointr_info(
    int cmd,
    aaiRecord *record_pointer,
    IOSCANPVT *scan
);
static long aai_record_read(aaiRecord *record_pointer);

// Need for I/O Intr scan test, delete after
static void start_iointr_worker_thread(initHookState state); 

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
    Aai aai_record(record_pointer);

#ifdef RECORD_DEBUG
    std::cout << aai_record.name() << " aai_init_record()" << std::endl << std::flush;
#endif

    AaiHandler *handler = new AaiHandler(
        aai_record.Record::raw(),
        true
    );
    aai_record.set_private_data((void *)handler);

    iointr::init_iointr_scan_lists();
    // Need for I/O Intr scan test, delete after
    initHookRegister(&start_iointr_worker_thread);

    return 0;
}

static long aai_get_iointr_info(
    int cmd,
    aaiRecord *record_pointer,
    IOSCANPVT *scan
) {
    Aai aai_record(record_pointer);

#ifdef RECORD_DEBUG
    std::cout << aai_record.name() << " aai_get_iointr_info(), command = "
    << cmd << std::endl << std::flush;
#endif

    iointr::init_scan_list(scan_list_name);
    *scan = iointr::get_scan_list(scan_list_name);
    
    return 0;
}

static long aai_record_read(aaiRecord *record_pointer) {
    Aai aai_record(record_pointer);

#ifdef RECORD_DEBUG
    std::cout << aai_record.name() << " aai_record_read() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    AaiHandler *handler = (AaiHandler *)aai_record.private_data();
    handler->epics_devsup_readwrite();

    return 0;
}


// Need for I/O Intr scan test, delete after
static void start_iointr_worker_thread(initHookState state) {
    if (state == initHookAfterInterruptAccept) {
#ifdef RECORD_DEBUG
    std::cout << "aaiDevSupp::start_iointr_worker_thread(), " << 
    "EPICS hook state = initHookAfterInterruptAccept" << std::endl
    << std::flush;
#endif
        iointr::start_scan_list_worker_thread(scan_list_name);
    }
}
