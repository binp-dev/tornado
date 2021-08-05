#pragma once

#include <mbboDirectRecord.h>

#include "base.hpp"

#include <record/value.hpp>

// TODO: derive from generic EpicsOutputValueRecord.
class MbboDirectRecord :
    public EpicsRecord,
    public virtual OutputValueRecord<uint16_t>
{
public:
    explicit MbboDirectRecord(mbboDirectRecord *raw) : EpicsRecord((dbCommon *)raw) {}

    mbboDirectRecord *raw() { 
        return (mbboDirectRecord *)EpicsRecord::raw(); 
    }
    const mbboDirectRecord *raw() const { 
    	return (const mbboDirectRecord *)EpicsRecord::raw(); 
    }

protected:
    const OutputValueHandler<uint16_t> *handler() const {
        return static_cast<const OutputValueHandler<uint16_t> *>(EpicsRecord::handler());
    }
    OutputValueHandler<uint16_t> *handler() {
        return static_cast<OutputValueHandler<uint16_t> *>(EpicsRecord::handler());
    }

    // TODO: Fix possible issues: https://epics-base.github.io/epics-base/mbboDirectRecord.html
    virtual void process_sync() override {
        if (handler() != nullptr) {
            handler()->write(*this);
        }
    }

public:
    virtual void set_handler(std::unique_ptr<OutputValueHandler<uint16_t>> &&handler) override {
        EpicsRecord::set_handler(std::move(handler));
    }

public:
    virtual uint16_t value() const override {
        return static_cast<uint16_t>(raw()->rval);
    }
};
