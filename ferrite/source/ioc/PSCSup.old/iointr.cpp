#include "iointr.hpp"

#include "core/assert.hpp"


void ScanList::destroy() {
    if (ioscan_list_ != nullptr) {
        // FIXME: Find a way to deinitialize IOSCANPVT
        core_panic("IOSCANPVT resource leak");
    }
}

ScanList::ScanList() {
    scanIoInit(&ioscan_list_);
}

ScanList::~ScanList() {
    destroy();
}

ScanList::ScanList(ScanList &&other) {
    ioscan_list_ = other.ioscan_list_;
    other.ioscan_list_ = nullptr;
}
ScanList &ScanList::operator=(ScanList &&other) {
    destroy();
    ioscan_list_ = other.ioscan_list_;
    other.ioscan_list_ = nullptr;
    return *this;
}

const IOSCANPVT &ScanList::raw() const {
    return ioscan_list_;
}

void ScanList::scan() {
    core_assert(ioscan_list_ != nullptr);
    // Runs record scanning in separate EPICS thread.
    // TODO: In case of performance issues consider using `scanIoImmediate` instead.
    scanIoRequest(ioscan_list_);
}
