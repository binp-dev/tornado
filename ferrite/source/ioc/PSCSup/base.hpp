#pragma once

#include <memory>
#include <optional>

#include <dbCommon.h>
#include <dbAccess.h>
#include <callback.h>

#include <core/assert.hpp>

#include <record/base.hpp>

#include "iointr.hpp"

// Record scan lock guard.
class ScanLockGuard final {
private:
    dbCommon *db_common_;

public:
    inline explicit ScanLockGuard(dbCommon *db_common) : db_common_(db_common) {
        dbScanLock(db_common_);
    }
    inline ~ScanLockGuard() {
        unlock();
    }

    inline void unlock() {
        if (db_common_ != nullptr) {
            dbScanUnlock(db_common_);
            db_common_ = nullptr;
        }
    }

    ScanLockGuard(const ScanLockGuard &) = delete;
    ScanLockGuard &operator=(const ScanLockGuard &) = delete;

    inline ScanLockGuard(ScanLockGuard &&other) : db_common_(other.db_common_) {
        other.db_common_ = nullptr;
    }
    inline ScanLockGuard &operator=(const ScanLockGuard &&other) {
        unlock();
        db_common_ = other.db_common_;
        return *this;
    }
};

// A persistent EPICS record wrapper and private data container.
template <typename R>
class EpicsRecord : public virtual Record {
private:
    // Raw record pointer. *Must be always valid.*
    // TODO: Does EPICS guarantee that this pointer has persistent address?
    R *const raw_;
    // Callback for asynchronous processing.
    epicsCallback async_callback_;
    // Scan list for I/O Intr. *Empty if record scanning is not `I/O Intr`.*
    std::optional<ScanList> scan_list_;

public:
    explicit EpicsRecord(R *raw) : raw_(raw) {
        init_async_processing_callback(&async_callback_);
    }
    virtual ~EpicsRecord() = default;

    // Record is non-copyable and non-movable.
    EpicsRecord(const EpicsRecord &) = delete;
    EpicsRecord &operator=(const EpicsRecord &) = delete;

    // Private data of the raw record stores `this`.
    static void set_private_data(R *raw, std::unique_ptr<EpicsRecord> &&record) {
        core_assert(raw->dpvt == nullptr);
        raw->dpvt = static_cast<void *>(record.release());
    }
    static const EpicsRecord *get_private_data(const R *raw) {
        core_assert(raw->dpvt != nullptr);
        const auto *record = (const EpicsRecord *)(raw->dpvt);
        // This assert fails if `raw` address change.
        core_assert(raw == record->raw());
        return record;
    }
    static EpicsRecord *get_private_data(R *raw) {
        core_assert(raw->dpvt != nullptr);
        auto *record = (EpicsRecord *)(raw->dpvt);
        // This assert fails if `raw` address change.
        core_assert(raw == record->raw());
        return record;
    }

protected:
    // Process record synchronously (usually calls handler).
    virtual void process_sync() = 0;
    // Pass I/O Intr callback to handler.
    virtual void register_processing_request() = 0;

private:
    // PACT flag for asynchronous record processing.
    bool is_processing_active() const {
        return raw_common()->pact != FALSE;
    }
    void set_processing_active(bool pact) {
        raw_common()->pact = pact ? TRUE : FALSE;
    }
    // Implicit record locking.
    [[nodiscard]] ScanLockGuard scan_lock() {
        return ScanLockGuard(raw_common());
    }

    // Internal methods for asynchronous processing pipeline.
    void notify_async_processing_complete() {
        auto rset = static_cast<struct typed_rset *>(raw_common()->rset);
        (*rset->process)(raw_common());
    }
    void schedule_async_processing() {
        callbackRequest(&async_callback_);
    }
    void process_async() {
        const auto guard = scan_lock();
        process_sync();
        notify_async_processing_complete();
    }
    static void async_processing_callback(epicsCallback *callback) {
        static_cast<EpicsRecord *>(callback->user)->process_async();
    }
    void init_async_processing_callback(epicsCallback *callback) {
        callbackSetCallback(&EpicsRecord::async_processing_callback, callback);
        callbackSetUser(static_cast<void *>(this), callback);
        callbackSetPriority(priorityLow, callback);
    }

public:
    // Raw record pointer.
    const R *raw() const {
        return raw_;
    }
    R *raw() {
        return raw_;
    }
    const dbCommon *raw_common() const {
        return (const dbCommon *)raw();
    }
    dbCommon *raw_common() {
        return (dbCommon *)raw();
    }

    // The entry point for record processing.
    void process() {
        if (this->handler() == nullptr) {
            return;
        }
        if (this->handler()->is_async) {
            if (!is_processing_active()) {
                set_processing_active(true);
                schedule_async_processing();
            } else {
                set_processing_active(false);
            }
        } else {
            process_sync();
        }
    }

    // Scan list of ther record (for I/O Intr).
    const std::optional<ScanList> &scan_list() const {
        return scan_list_;
    }
    std::optional<ScanList> &scan_list() {
        return scan_list_;
    }
    void set_scan_list(std::optional<ScanList> &&scan_list) {
        scan_list_ = std::move(scan_list);
        register_processing_request();
    }

    // Initiate request processing (call I/O Intr).
    void request_processing() {
        core_assert(scan_list_.has_value());
        scan_list_->scan();
    }

public:
    virtual std::string_view name() const override {
        return std::string_view(raw_common()->name);
    }
};
