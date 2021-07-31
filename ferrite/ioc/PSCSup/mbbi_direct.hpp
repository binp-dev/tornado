#pragma once

#include <mbbiDirectRecord.h>

#include "base.hpp"

#include <record/value.hpp>

class MbbiDirectRecord :
    public EpicsRecord,
    public virtual InputValueRecord<uint16_t>
{
public:
    explicit MbbiDirectRecord(mbbiDirectRecord *raw) : EpicsRecord((dbCommon *)raw) {}

    mbbiDirectRecord *raw() { 
        return (mbbiDirectRecord *)EpicsRecord::raw(); 
    }
    const mbbiDirectRecord *raw() const { 
    	return (const mbbiDirectRecord *)EpicsRecord::raw(); 
    }

protected:
    const InputValueHandler<uint16_t> *handler() const {
        return static_cast<const InputValueHandler<uint16_t> *>(EpicsRecord::handler());
    }
    InputValueHandler<uint16_t> *handler() {
        return static_cast<InputValueHandler<uint16_t> *>(EpicsRecord::handler());
    }

    virtual void process_sync() override {
        if (handler() != nullptr) {
            handler()->read(*this);
        }
    }

public:
    virtual void set_handler(std::unique_ptr<InputValueHandler<uint16_t>> &&handler) override {
        EpicsRecord::set_handler(std::move(handler));
    }

public:
    virtual uint16_t value() const {
        return static_cast<uint16_t>(raw()->rval);
    }
};

class MbbiDirectHandler : public InputValueHandler<uint16_t> {
public:
    virtual void read(InputValueRecord<uint16_t> &record) = 0;
};
