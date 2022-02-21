#pragma once

#include <mbbiDirectRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class MbbiDirectRecord final :
    public EpicsInputValueRecord<uint32_t, mbbiDirectRecord>,
    public virtual InputValueRecord<uint32_t> //
{
public:
    inline explicit MbbiDirectRecord(mbbiDirectRecord *raw) : EpicsInputValueRecord<uint32_t, mbbiDirectRecord>(raw) {}

    [[nodiscard]] virtual uint32_t value() const override;
    virtual void set_value(uint32_t value) override;
};
