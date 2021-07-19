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
    std::cout << mbbo_direct_record.name() << " mbboDirect_init_record()" << 
    std::endl << std::flush;
#endif

    MbboDirectHandler *handler = new MbboDirectHandler(
        mbbo_direct_record.Record::raw(),
        true
    );
    mbbo_direct_record.set_private_data((void *)handler);

    return 0;
}

static long mbboDirect_record_write(mbboDirectRecord *record_pointer) {
    MbboDirect mbbo_direct_record(record_pointer);
    
#ifdef RECORD_DEBUG
    std::cout << mbbo_direct_record.name() << 
    " mbboDirect_record_write() Thread id = " <<
    pthread_self() << std::endl << std::flush;
#endif
    
    MbboDirectHandler *handler = (MbboDirectHandler *)mbbo_direct_record.private_data();
    handler->epics_devsup_readwrite();

    return 0;
}