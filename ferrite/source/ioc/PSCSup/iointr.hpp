#pragma once

#include <dbScan.h>

class ScanList final {
private:
    IOSCANPVT ioscan_list_;
    void destroy();

public:
    ScanList();
    ~ScanList();

    ScanList(const ScanList &) = delete;
    ScanList &operator=(const ScanList &) = delete;

    ScanList(ScanList &&other);
    ScanList &operator=(ScanList &&other);

    const IOSCANPVT &raw() const;

    void scan();
};
