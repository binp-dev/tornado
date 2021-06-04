#pragma once

#include <typeinfo>

#include <aaiRecord.h>
#include <cantProceed.h>
#include <dbAccess.h>

#include "base.hpp"

template <typename aaType>
class AnalogArray : public Record {
public:
    virtual ~AnalogArray() override = default;


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
    public ReadableRecord 
{
public:
    explicit AnalogArrayInput(aaiRecord *raw);
    virtual ~AnalogArrayInput() override = default;

    virtual void read() override;
};