#pragma once

#include <cantProceed.h>
#include <dbAccess.h>
#include <aaiRecord.h>
#include <aaoRecord.h>
#include <epicsTypes.h>

#include "core/assert.hpp"
#include "base.hpp"

template <typename aaType>
class AnalogArray : public Record {
public:
    virtual ~AnalogArray() override = default;

    epicsUInt32 length() { return raw()->nord; }
    epicsUInt32 length() const { return raw()->nord; };
    void set_length(epicsUInt32 nord) { raw()->nord = nord; }

    epicsUInt32 max_length() { return raw()->nelm; }
    epicsUInt32 max_length() const { return raw()->nelm; };

    void *raw_data() { return raw()->bptr; }
    void *raw_data() const { return raw()->bptr; }

    template <typename T>
    T *array_data() { return (T *)raw_data(); }

    template <typename T>
    const T *array_data() const { return (const T *)raw_data(); }
protected:
    explicit AnalogArray(aaType *raw) : Record((dbCommon *)raw) {
        allocate_data_buff();
    }
    
    void allocate_data_buff() {
        if (raw()->bptr != nullptr) { return; }
        assert_true(raw()->nelm != 0);

        raw()->bptr = callocMustSucceed(
            raw()->nelm,
            dbValueSize(raw()->ftvl), 
            "Can't allocate memory for data buffer of AnalogArray record."
        );
    }

    aaType *raw() { return (aaType *)Record::raw(); }
    const aaType *raw() const { return (const aaType *)Record::raw(); }
};

class Aai final : public AnalogArray<aaiRecord> {
public:
    explicit Aai(aaiRecord *raw);
    virtual ~Aai() override = default;
};


class AaiHandler final : public Handler {
public:
    AaiHandler(dbCommon *raw_record);
    AaiHandler(
        dbCommon *raw_record, 
        bool asyn_process
    );
    virtual ~AaiHandler() override = default;

    AaiHandler(const AaiHandler &) = delete;
    AaiHandler &operator=(const AaiHandler &) = delete;
    AaiHandler(AaiHandler &&) = default;
    AaiHandler &operator=(AaiHandler &&) = default;

    virtual void readwrite() override;
};


class Aao final : public AnalogArray<aaoRecord> {
public:
    explicit Aao(aaoRecord *raw);
    virtual ~Aao() override = default;
};


class AaoHandler final : public Handler {
public:
    AaoHandler(dbCommon *raw_record);
    AaoHandler(
        dbCommon *raw_record, 
        bool asyn_process
    );
    virtual ~AaoHandler() override = default;

    AaoHandler(const AaoHandler &) = delete;
    AaoHandler &operator=(const AaoHandler &) = delete;
    AaoHandler(AaoHandler &&) = default;
    AaoHandler &operator=(AaoHandler &&) = default;

    virtual void readwrite() override;
};