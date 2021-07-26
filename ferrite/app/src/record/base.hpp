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


class Handler {
public:
    virtual ~Handler() = default;

    Handler(const Handler &) = delete;
    Handler &operator=(const Handler &) = delete;
    Handler(Handler &&) = default;
    Handler &operator=(Handler &&) = default;

    // Function that encapsulate EPICS logic of read or write function.
    // This function used in record device support. 
    void epics_devsup_readwrite();

    // Function that implements the logic of reading/writing data from/to device.
    virtual void readwrite() = 0;


    // Function that encapsulate EPICS logic of read or write callback function.
    // This function used for asynchronous reading/writing mode.
    static void epics_readwrite_callback(CALLBACK *callback_struct_pointer);
protected:
    Handler(dbCommon *raw_record, bool asyn_process = false);

    bool asyn_process;
    dbCommon *raw_record_;
};
