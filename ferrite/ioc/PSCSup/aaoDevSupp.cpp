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
#include <aaoRecord.h>

#include <pthread.h>

#include "record/analogArrayIO.hpp"


static long aao_init_record(aaoRecord *record_pointer);
static long aao_record_write(aaoRecord *record_pointer);

struct AaoDevSuppSet {
    long num;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_iointr_info;
    DEVSUPFUN write_aao;
};

static struct AaoDevSuppSet aao_device_support_handler = {
    5,
    NULL,
    NULL,
    reinterpret_cast<DEVSUPFUN>(aao_init_record),
    NULL,
    reinterpret_cast<DEVSUPFUN>(aao_record_write)
};

epicsExportAddress(dset, aao_device_support_handler);


static long aao_init_record(aaoRecord *record_pointer) {
    Aao aao_record(record_pointer);

#ifdef RECORD_DEBUG
    std::cout << aao_record.name() << " aao_init_record()" << std::endl << std::flush;
#endif

    AaoHandler *handler = new AaoHandler(
        aao_record.Record::raw(),
        true
    );
    aao_record.set_private_data((void *)handler);


    return 0;
}

static long aao_record_write(aaoRecord *record_pointer) {
    Aao aao_record(record_pointer);

#ifdef RECORD_DEBUG
    std::cout << aao_record.name() << " aao_record_write() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    AaoHandler *handler = (AaoHandler *)aao_record.private_data();
    handler->epics_devsup_readwrite();

    return 0;
}
