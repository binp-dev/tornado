#pragma once

#include <dbCommon.h>
#include <callback.h>

#include "recordDebugBuild.hpp"

typedef void(*callback_function)(CALLBACK *);

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

    bool get_pact();
    bool get_pact() const;
    void set_pact(bool pact);
    struct typed_rset *get_rset();

    void scan_lock();
    void scan_unlock();
    void process_record();

    void set_callback(callback_function callback);
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

    Record::PrivateData *get_dptr_struct();
    Record::PrivateData *get_dptr_struct() const;
public:
    const dbCommon *raw() const;
    dbCommon *raw();

    void set_private_data(void *data);
    const void *get_private_data() const;
    void *get_private_data();
};

class Handler {
public:
    Handler() = default;
    virtual ~Handler() = default;

    Handler(const Handler &) = delete;
    Handler &operator=(const Handler &) = delete;
    Handler(Handler &&) = default;
    Handler &operator=(Handler &&) = default;
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