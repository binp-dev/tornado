#include "iointr.hpp"

#include <iostream>
#include <map>

#include <cantProceed.h>
#include <epicsThread.h>
#include <callback.h>

#include "core/assert.hpp"
#include "core/lazy_static.hpp"


namespace iointr {

void init_map(MaybeUninit<std::map<std::string, IOSCANPVT *>> &mem) {
    std::cout << "Init lazy static SCAN_LISTS" << std::endl;
    mem.init_in_place();
}

LazyStatic<std::map<std::string, IOSCANPVT *>, init_map> SCAN_LISTS = {};

void init_iointr_scan_lists() {
    // Explicitly initialize device.
    *SCAN_LISTS;
}

void init_scan_list(const std::string &list_name) {
    if ((*SCAN_LISTS).count(list_name) == 1) { return; }

    IOSCANPVT *ioscan_list_ptr = (IOSCANPVT *)callocMustSucceed(
        1, 
        sizeof(IOSCANPVT), 
        "iointr::init_scan_list: Can't allocate memory for IOSCANPVT"
    );
    scanIoInit(ioscan_list_ptr);
    (*SCAN_LISTS)[list_name] = ioscan_list_ptr;
}

IOSCANPVT &get_scan_list(const std::string &list_name) {
    assert_true((*SCAN_LISTS).count(list_name) == 1);
    return *((*SCAN_LISTS)[list_name]);
}


void worker(void *args) {
    std::cout << "This is worker" << std::endl;

   IOSCANPVT scan = iointr::get_scan_list("TEST_SCAN_LIST");
    while (true) {
#ifdef RECORD_DEBUG
            std::cout << "INIT RECORD PROCESSING FOR SCAN LIST \"" 
            << scan_list_name << "\", FROM Thread id = " << pthread_self() 
            << std::endl << std::flush;
#endif
        scanIoImmediate(scan, priorityLow);
        scanIoImmediate(scan, priorityHigh);
        scanIoImmediate(scan, priorityMedium);

        epicsThreadSleep(1.0);
    }


    return;
}

void start_scan_list_worker_thread(std::string list_name) {
    assert((*SCAN_LISTS).count(list_name) != 0);

    epicsThreadMustCreate(
        list_name.c_str(),
        epicsThreadPriorityHigh,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        worker, 
        nullptr
    );

}

} // namespace iointr