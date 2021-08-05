#pragma once

#include <memory>

#include <dbCommon.h>
#include <callback.h>

#include <record/base.hpp>

#include "iointr.hpp"

// Record scan lock guard.
class ScanLockGuard final {
private:
    dbCommon *db_common_;

public:
    explicit ScanLockGuard(dbCommon *db_common);
    ~ScanLockGuard();

    void unlock();

    ScanLockGuard(const ScanLockGuard &) = delete;
    ScanLockGuard &operator=(const ScanLockGuard &) = delete;

    ScanLockGuard(ScanLockGuard &&other);
    ScanLockGuard &operator=(const ScanLockGuard &&other);
};

// A persistent EPICS record wrapper and private data container.
class EpicsRecord : public virtual Record
{
private:
    // Raw record pointer. *Must be always valid.*
    // TODO: Does EPICS guarantee that this pointer has persistent address?
    dbCommon *const raw_;
    // Callback for asynchronous processing.
    epicsCallback async_callback_;
    // User-defined handler for the record. *Can be empty.*
    std::unique_ptr<Handler> handler_;
    // Scan list for I/O Intr. *Empty if record scanning is not `I/O Intr`.*
    std::optional<ScanList> scan_list_;

public:
    explicit EpicsRecord(dbCommon *raw);
    virtual ~EpicsRecord() = default;

    // Record is non-copyable and non-movable.
    EpicsRecord(const EpicsRecord &) = delete;
    EpicsRecord &operator=(const EpicsRecord &) = delete;

    // Private data of the raw record stores `this`.
    static void set_private_data(dbCommon *raw, std::unique_ptr<EpicsRecord> &&record);
    static const EpicsRecord *get_private_data(const dbCommon *raw);
    static EpicsRecord *get_private_data(dbCommon *raw);

protected:
    // Process record synchronously (usually calls handler).
    virtual void process_sync() = 0;
    // Pass I/O Intr callback to handler.
    virtual void register_processing_request();

private:
    // PACT flag for asynchronous record processing.
    bool is_processing_active() const;
    void set_processing_active(bool pact);
    // Implicit record locking.
    [[nodiscard]] ScanLockGuard scan_lock();

    // Internal methods for asynchronous processing pipeline. 
    void notify_async_processing_complete();
    void schedule_async_processing();
    void process_async();
    static void async_processing_callback(epicsCallback *callback);
    void init_async_processing_callback(epicsCallback *callback);

public:
    // Raw record pointer.
    const dbCommon *raw() const;
    dbCommon *raw();

    // Record handler pointer (can be `nullptr`).
    const Handler *handler() const;
    Handler *handler();

    // The entry point for record processing.
    void process();

    // Scan list of ther record (for I/O Intr).
    const std::optional<ScanList> &scan_list() const;
    std::optional<ScanList> &scan_list();
    void set_scan_list(std::optional<ScanList> &&scan_list);

    // Initiate request processing (call I/O Intr).
    void request_processing();

public:
    virtual std::string_view name() const override;

protected:
    void set_handler(std::unique_ptr<Handler> &&handler);
};
