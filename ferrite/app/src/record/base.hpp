#pragma once

#include <functional>

#include <dbCommon.h>
#include <callback.h>

#include "recordDebugBuild.hpp"

typedef void (callback_func_t)(CALLBACK *);

class Record {
private:
    dbCommon *raw_;
public:
    inline explicit Record(dbCommon *raw) : raw_(raw) {
        if (raw_->dpvt == nullptr) {
            raw_->dpvt = static_cast<void *>(new Record::PrivateData());
        }
    }
    virtual ~Record() = default;

    Record(const Record &) = delete;
    Record &operator=(const Record &) = delete;
    Record(Record &&) = delete;
    Record &operator=(Record &&) = delete;

    const char *name() const;

    bool pact();
    bool pact() const;
    void set_pact(bool pact);
    struct typed_rset *rset();

    void scan_lock();
    void scan_unlock();
    void process_record();

    void set_callback(std::function<callback_func_t> callback);
    void request_callback();
protected:
    struct PrivateData final {
    public:
        CALLBACK *callback_struct_ptr = nullptr;
        void *data = nullptr;
        
        explicit PrivateData() = default;
        PrivateData(const PrivateData &) = delete;
        PrivateData(PrivateData &&) = delete;
        ~PrivateData() = default;
        PrivateData &operator=(const PrivateData &) = default;
        PrivateData &operator=(PrivateData &&) = delete;
    };

    Record::PrivateData *dptr_struct();
    Record::PrivateData *dptr_struct() const;
public:
    const dbCommon *raw() const;
    dbCommon *raw();

    void set_private_data(void *data);
    const void *private_data() const;
    void *private_data();
};


class InputRecord {
public:
    InputRecord() = default;
    InputRecord(const InputRecord &) = delete;
    InputRecord(InputRecord &&) = delete;

    InputRecord &operator=(const InputRecord &) = delete;
    InputRecord &operator=(InputRecord &&) = delete;
    
    virtual ~InputRecord() = default;

    virtual void read() = 0;
};


class OutputRecord {
public:
    OutputRecord() = default;
    OutputRecord(const OutputRecord &) = delete;
    OutputRecord(OutputRecord &&) = delete;

    OutputRecord &operator=(const OutputRecord &) = delete;
    OutputRecord &operator=(OutputRecord &&) = delete;
    
    virtual ~OutputRecord() = default;

    virtual void write() = 0;
};


//--------------------------------------------------------

class Handler {
public:
    virtual ~Handler() = default;

    Handler(const Handler &) = delete;
    Handler &operator=(const Handler &) = delete;
    Handler(Handler &&) = default;
    Handler &operator=(Handler &&) = default;

    void epics_read_write();
    static void epics_read_write_callback(CALLBACK *callback_struct_pointer);
protected:
    Handler(dbCommon *raw_record, std::function<void()> read_write);
    Handler(
        dbCommon *raw_record,
        bool asyn_process,
        std::function<void()> read_write
    );

    bool asyn_process;
    dbCommon *raw_record_;
public: // protected???
    std::function<void()> read_write;
};


class ReadHandler : public Handler {
public:
    ReadHandler(dbCommon *raw_record);
    ReadHandler(dbCommon *raw_record, bool asyn_process);
    virtual ~ReadHandler() = default;

    ReadHandler(const ReadHandler &) = delete;
    ReadHandler &operator=(const ReadHandler &) = delete;
    ReadHandler(ReadHandler &&) = default;
    ReadHandler &operator=(ReadHandler &&) = default;

    virtual void read() = 0;
};


class WriteHandler : public Handler {
public:
    WriteHandler(dbCommon *raw_record);
    WriteHandler(dbCommon *raw_record, bool asyn_process);
    virtual ~WriteHandler() = default;

    WriteHandler(const WriteHandler &) = delete;
    WriteHandler &operator=(const WriteHandler &) = delete;
    WriteHandler(WriteHandler &&) = default;
    WriteHandler &operator=(WriteHandler &&) = default;

    virtual void write() = 0;
};


