#pragma once

#include <mbboDirectRecord.h>

#include "base.hpp"

#include <record/value.hpp>

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

    virtual void process_sync() override {
        if (handler() != nullptr) {
            handler()->write(*this);
        }
    }

    virtual void register_processing_request() override {
        if (handler() != nullptr) {
            handler()->set_write_request(*this, [this]() {
                request_processing();
            });
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
    virtual void set_value(uint16_t value) override {
        raw()->rval = static_cast<epicsUInt32>(value);
    }
};

class MbbiDirectHandler : public OutputValueHandler<uint16_t> {
public:
    virtual void write(OutputValueRecord<uint16_t> &record) = 0;
};
