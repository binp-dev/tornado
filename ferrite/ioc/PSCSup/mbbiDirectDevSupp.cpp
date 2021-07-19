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


static long mbbiDirect_init_record(mbbiDirectRecord *record_pointer);
static long mbbiDirect_get_iointr_info(
    int cmd,
    mbbiDirectRecord *record_pointer,
    IOSCANPVT *scan
);
static long mbbiDirect_record_read(mbbiDirectRecord *record_pointer);

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

    MbbiDirectHandler *handler = new MbbiDirectHandler(
        mbbi_direct_record.Record::raw(),
        true
    );
    mbbi_direct_record.set_private_data((void *)handler);

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

    MbbiDirectHandler *handler = (MbbiDirectHandler *)mbbi_direct_record.private_data();
    handler->epics_devsup_readwrite();
    
    return 0;
}