#pragma once

#include <map>
#include <string>
#include <utility>

#include <epicsThread.h>
#include <dbScan.h>


typedef void(*worker_func)(void *);

/*
Namespace with functions for work witch EPICS scan mode "I/O Intr". This functions is 
used to initialize scan lists from different DeviceSupport and start worker threads
in which events associated with the corresponding scan list should be checked.
*/
namespace iointr {


struct ScanListData final {
public:
    IOSCANPVT *ioscan_list_ptr;
    worker_func worker;
    void *worker_args;
    epicsThreadId worker_thread_id;

    explicit ScanListData() = default;
    ScanListData(const ScanListData &) = default;
    ScanListData(ScanListData &&) = delete;
    ~ScanListData() = default;
    ScanListData &operator=(const ScanListData &) = default;
    ScanListData &operator=(ScanListData &&) = delete;
};

/*
Init EPICS scan list and save associated name and worker thread.
If scan list with that name already exist, then nothing happend. 
*/
void init_scan_list(
    const std::string &list_name,
    worker_func worker,
    void *worker_args
);

IOSCANPVT &get_scan_list(const std::string &list_name);

/*
Start worker thread, associated with scan list.
If thread already started, when nothing happend.
*/
void start_scan_list_worker_thread(const std::string &list_name);


} // namespace ioscan

