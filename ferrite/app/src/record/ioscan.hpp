#pragma once

#include <map>
#include <string>
#include <utility>

#include <dbScan.h>
#include <epicsThread.h>

typedef void(*worker_func)(void *);

class IOScan {
public:
    static void init_ioscan_list(
        const std::string &list_name,
        worker_func worker,
        void *worker_args
    );
    static IOSCANPVT &get_ioscan_list(const std::string &list_name);
    static void start_ioscan_list_worker(const std::string &list_name);
private:
    struct IOScanListData final {
    public:
        IOSCANPVT *ioscan_list_ptr;
        worker_func worker;
        void *worker_args;
        epicsThreadId worker_thread_id;

        explicit IOScanListData() = default;
        IOScanListData(const IOScanListData &) = default;
        IOScanListData(IOScanListData &&) = delete;
        ~IOScanListData() = default;
        IOScanListData &operator=(const IOScanListData &) = default;
        IOScanListData &operator=(IOScanListData &&) = delete;
    };

    static std::map<std::string, IOScan::IOScanListData> ioscan_lists_data;
};

