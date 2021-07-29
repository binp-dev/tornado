#pragma once

#include <memory>

#define USE_TYPED_RSET

#include <dbCommon.h>
#include <callback.h>

#include <record/base.hpp>

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

// A *persistent* EPICS record wrapper and private data container.
class EpicsRecord : public virtual Record
{
private:
    dbCommon *const raw_;
    epicsCallback async_callback_;
    std::unique_ptr<Handler> handler_;

public:
    explicit EpicsRecord(dbCommon *raw);
    virtual ~EpicsRecord() = default;

    // Record is non-copyable and non-movable.
    EpicsRecord(const EpicsRecord &) = delete;
    EpicsRecord &operator=(const EpicsRecord &) = delete;

    static void set_private_data(dbCommon *raw, std::unique_ptr<EpicsRecord> &&record);
    static const EpicsRecord *get_private_data(const dbCommon *raw);
    static EpicsRecord *get_private_data(dbCommon *raw);

protected:
    virtual void process_sync() = 0;

private:
    bool is_process_active() const;
    void set_process_active(bool pact);
    [[nodiscard]] ScanLockGuard scan_lock();

    void notify_async_process_complete();
    void schedule_async_process();
    void process_async();
    static void async_process_callback(epicsCallback *callback);
    void init_async_process_callback(epicsCallback *callback);

public:
    const dbCommon *raw() const;
    dbCommon *raw();

    const Handler *handler() const;
    Handler *handler();

    void process();

public:
    virtual std::string_view name() const override;

protected:
    void set_handler(std::unique_ptr<Handler> &&handler);
};
