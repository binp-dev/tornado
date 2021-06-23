#include "iointr.hpp"

#include <iostream>

#include <cantProceed.h>


namespace iointr {


// global static map, that contains EPICS scan lists and associated data.
static std::map<std::string, ScanListData> scan_lists;


void init_scan_list(
    const std::string &list_name,
    worker_func worker,
    void *worker_args
) {
    if (scan_lists.count(list_name) == 1) { return; }
    assert(worker != nullptr);

    IOSCANPVT *ioscan_list_ptr = (IOSCANPVT *)callocMustSucceed(
        1, 
        sizeof(IOSCANPVT), 
        "iointr::init_scan_list: Can't allocate memory for IOSCANPVT"
    );
    scanIoInit(ioscan_list_ptr);

    iointr::ScanListData list_data;
    list_data.ioscan_list_ptr = ioscan_list_ptr;

    list_data.worker = worker;
    list_data.worker_args = worker_args;
    list_data.worker_thread_id = 0;
    
    scan_lists[list_name] = list_data;
}

IOSCANPVT &get_scan_list(const std::string &list_name) {
    assert(scan_lists.count(list_name) == 1);
    return *(scan_lists[list_name].ioscan_list_ptr);
}

void start_scan_list_worker_thread(const std::string &list_name) {
    assert(scan_lists.count(list_name) != 0);

    iointr::ScanListData list_data = scan_lists[list_name];
    if (list_data.worker_thread_id != 0) { 
        return;
    }
    assert(list_data.worker != nullptr);
    
    scan_lists[list_name].worker_thread_id = epicsThreadMustCreate(
        list_name.c_str(),
        epicsThreadPriorityHigh,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        list_data.worker, 
        list_data.worker_args
    );
}


} // namespace ioscan