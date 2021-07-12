#pragma once

#include <cantProceed.h>
#include <dbAccess.h>
#include <aaiRecord.h>
#include <aaoRecord.h>

#include "base.hpp"

template <typename aaType>
class AnalogArray : public Record {
public:
    virtual ~AnalogArray() override = default;

    unsigned long length() { return raw()->nord; }
    unsigned long length() const { return raw()->nord; };
    void set_length(unsigned long nord) { raw()->nord = nord; }

    unsigned long max_length() { return raw()->nelm; }
    unsigned long max_length() const { return raw()->nelm; };

    void *raw_data() { return raw()->bptr; }
    void *raw_data() const { return raw()->bptr; }
protected:
    explicit AnalogArray(aaType *raw) : Record((dbCommon *)raw) {
        allocate_data_buff();
    }
    
    void allocate_data_buff() {
        if (raw()->bptr != nullptr) { return; }
        assert(raw()->nelm != 0);

        raw()->bptr = callocMustSucceed(
            raw()->nelm,
            dbValueSize(raw()->ftvl), 
            "Can't allocate memory for data buffer of AnalogArray record."
        );
    }

    aaType *raw() { return (aaType *)Record::raw(); }
    const aaType *raw() const { return (const aaType *)Record::raw(); }
};

class AnalogArrayInput final :
    public AnalogArray<aaiRecord>, 
    public InputRecord {
public:
    explicit AnalogArrayInput(aaiRecord *raw);
    virtual ~AnalogArrayInput() override = default;

    virtual void read() override;
};

class AnalogArrayOutput final :
    public AnalogArray<aaoRecord>, 
    public OutputRecord {
public:
    explicit AnalogArrayOutput(aaoRecord *raw);
    virtual ~AnalogArrayOutput() override = default;

    virtual void write() override;
};