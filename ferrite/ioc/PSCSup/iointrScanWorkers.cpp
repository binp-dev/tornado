#include "iointrScanWorkers.hpp"

#include <iostream>

#include <pthread.h>

#include <callback.h>

#include "record/recordDebugBuild.hpp"
#include "record/iointr.hpp" 


const std::string scan_list_name = "TEST_SCAN_LIST";

void iointr_worker(void *args) {
#ifdef RECORD_DEBUG
	std::cout << "START WORKING THREAD FOR SCAN LIST \"" << scan_list_name <<
	"\", Thread id = " << pthread_self() << std::endl << std::flush;
#endif

    IOSCANPVT scan = iointr::get_scan_list(scan_list_name);
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
}