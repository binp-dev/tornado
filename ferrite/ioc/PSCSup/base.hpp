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

// A non-owning wrapper around EPICS record.
class EpicsRecord : public virtual Record
{
protected:
    struct PrivateData {
        std::unique_ptr<Handler> handler;
        epicsCallback async_callback_data;
    };

private:
    dbCommon *const raw_;

public:
    explicit EpicsRecord(dbCommon *raw);
    virtual ~EpicsRecord() = default;

    // Record is non-copyable and non-movable.
    EpicsRecord(const EpicsRecord &) = delete;
    EpicsRecord &operator=(const EpicsRecord &) = delete;

protected:
    virtual void process_sync() = 0;

private:
    void set_private_data(std::unique_ptr<PrivateData> &&data);
    const PrivateData &private_data() const;
    PrivateData &private_data();

    bool is_process_active() const;
    void set_process_active(bool pact);
    [[nodiscard]] ScanLockGuard scan_lock();

    void notify_async_process_complete();
    void schedule_async_process();
    void process_async();
    static void async_process_callback(epicsCallback *callback);
    [[nodiscard]] epicsCallback make_async_process_callback();

public:
    const dbCommon *raw() const;
    dbCommon *raw();

    void initialize();

    const Handler *handler() const;
    Handler *handler();

    void process();

public:
    virtual std::string_view name() const override;

protected:
    void set_handler(std::unique_ptr<Handler> &&handler);
};
