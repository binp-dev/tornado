#include <iostream>
#include <cantProceed.h>

#include "ioscan.hpp"

std::map<std::string, IOScan::IOScanListData> IOScan::ioscan_lists_data;

void IOScan::init_ioscan_list(
    const std::string &list_name,
    worker_func worker,
    void *worker_args
) {
    if (ioscan_lists_data.count(list_name) == 1) { return; }

    std::cout << "init_ioscan_list()" << std::endl << std::flush;
    IOSCANPVT *ioscan_list_ptr = (IOSCANPVT *)callocMustSucceed(
        1, 
        sizeof(IOSCANPVT), 
        "IOScan::init_ioscan_list: Can't allocate memory for IOSCANPVT"
    );
    scanIoInit(ioscan_list_ptr);

    IOScan::IOScanListData list_data;
    list_data.ioscan_list_ptr = ioscan_list_ptr;
    std::cout << "list_data.ioscan_list_ptr = " << list_data.ioscan_list_ptr << std::endl << std::flush;

    list_data.worker = worker;
    list_data.worker_args = worker_args;
    list_data.worker_thread_id = 0;
    
    ioscan_lists_data[list_name] = list_data;
}

IOSCANPVT &IOScan::get_ioscan_list(const std::string &list_name) {
    assert(ioscan_lists_data.count(list_name) == 1);
    std::cout << "get_ioscan_list()" << std::endl << std::flush;
    std::cout << "list_data.ioscan_list_ptr = " << ioscan_lists_data[list_name].ioscan_list_ptr << std::endl << std::flush;
    return *(ioscan_lists_data[list_name].ioscan_list_ptr);
}

void IOScan::start_ioscan_list_worker(const std::string &list_name) {
    assert(ioscan_lists_data.count(list_name) == 1);

    IOScan::IOScanListData list_data = ioscan_lists_data[list_name];
    if (list_data.worker_thread_id != 0) { return; }
    
    assert(list_data.worker != nullptr);
    
    list_data.worker_thread_id = epicsThreadMustCreate(
        list_name.c_str(),
        epicsThreadPriorityHigh,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        list_data.worker, 
        list_data.worker_args
    );
}