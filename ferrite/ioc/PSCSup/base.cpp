#include "base.hpp"

#include <core/assert.hpp>

#include <dbAccess.h>

ScanLockGuard::ScanLockGuard(dbCommon *db_common) : db_common_(db_common) {
    dbScanLock(db_common_);
}
ScanLockGuard::~ScanLockGuard() {
    unlock();
}

void ScanLockGuard::unlock() {
    if (db_common_ != nullptr) {
        dbScanUnlock(db_common_);
        db_common_ = nullptr;
    }
}

ScanLockGuard::ScanLockGuard(ScanLockGuard &&other) : db_common_(other.db_common_) {
    other.db_common_ = nullptr;
}
ScanLockGuard &ScanLockGuard::operator=(const ScanLockGuard &&other) {
    unlock();
    db_common_ = other.db_common_;
    return *this;
}


EpicsRecord::EpicsRecord(dbCommon *raw) : raw_(raw) {}

void EpicsRecord::set_private_data(std::unique_ptr<PrivateData> &&data) {
    assert_true(raw()->dpvt == nullptr);
    raw()->dpvt = static_cast<void *>(data.release());
}
const EpicsRecord::PrivateData &EpicsRecord::private_data() const {
    return *static_cast<const PrivateData *>(raw()->dpvt);
}
EpicsRecord::PrivateData &EpicsRecord::private_data() {
    return *static_cast<PrivateData *>(raw()->dpvt);
}

bool EpicsRecord::is_process_active() const {
    return raw()->pact != FALSE;
}
void EpicsRecord::set_process_active(bool pact) {
    raw()->pact = pact ? TRUE : FALSE;
}

ScanLockGuard EpicsRecord::scan_lock() {
    return ScanLockGuard(raw());
}

void EpicsRecord::notify_async_process_complete() {
    auto rset = static_cast<struct typed_rset *>(raw()->rset);
    (*rset->process)(raw());
}

void EpicsRecord::schedule_async_process() {
    callbackRequest(&private_data().async_callback_data);
}

void EpicsRecord::process_async() {
    const auto guard = scan_lock();
    process_sync();
    notify_async_process_complete();
}

epicsCallback EpicsRecord::make_async_process_callback() {
    epicsCallback callback;

    callbackSetCallback(get_async_process_callback(), &callback);
    callbackSetUser(static_cast<void *>(raw()), &callback);
    callbackSetPriority(priorityLow, &callback);

    return callback;
}

const dbCommon *EpicsRecord::raw() const {
    return raw_;
}
dbCommon *EpicsRecord::raw() {
    return raw_;
}

void EpicsRecord::initialize() {
    auto private_data = std::make_unique<PrivateData>();
    private_data->async_callback_data = make_async_process_callback();
    set_private_data(std::move(private_data));
}

const Handler *EpicsRecord::handler() const {
    return private_data().handler.get();
}
Handler *EpicsRecord::handler() {
    return private_data().handler.get();
}

void EpicsRecord::process() {
    if (handler() == nullptr) {
        return;
    }
    if (handler()->is_async()) {
        if (!is_process_active()) {
            set_process_active(true);
            schedule_async_process();
        } else {
            set_process_active(false);
        }
    } else {
        process_sync();
    }
}

std::string_view EpicsRecord::name() const {
    return std::string_view(raw()->name);
}

void EpicsRecord::set_handler(std::unique_ptr<Handler> &&handler) {
    private_data().handler = std::move(handler);
}
