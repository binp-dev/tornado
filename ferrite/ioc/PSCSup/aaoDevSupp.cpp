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

static void aao_record_write_callback(CALLBACK *callback_pointer);

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
    AnalogArrayOutput aaoRec(record_pointer);
#ifdef RECORD_DEBUG
    std::cout << aaoRec.name() << " aao_init_record()" << std::endl << std::flush;
#endif
    aaoRec.set_callback(aao_record_write_callback);

    return 0;
}

static long aao_record_write(aaoRecord *record_pointer) {
    AnalogArrayOutput aaoRec(record_pointer);
#ifdef RECORD_DEBUG
    std::cout << aaoRec.name() << " aao_record_write() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    if (aaoRec.pact() != true) {
        aaoRec.set_pact(true);
        aaoRec.request_callback();
    }
    
    return 0;
}

static void aao_record_write_callback(CALLBACK *callback_struct_pointer) {
    struct dbCommon *record_pointer;
    // Line below doesn't work, because in C++ cast result is not lvalue
    // callbackGetUser((void *)(record_pointer), callback_struct_pointer);
    record_pointer = (dbCommon *)callback_struct_pointer->user;

    AnalogArrayOutput aaoRec((aaoRecord *)record_pointer);
#ifdef RECORD_DEBUG
    std::cout << aaoRec.name() << " aao_record_write_callback() Thread id = " << 
    pthread_self() << std::endl << std::flush;
#endif

    aaoRec.scan_lock();
    aaoRec.write();
    aaoRec.process_record();
    aaoRec.scan_unlock();
}