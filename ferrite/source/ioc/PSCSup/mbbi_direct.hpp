#pragma once

#include <mbbiDirectRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class MbbiDirectRecord :
    public EpicsInputValueRecord<uint32_t, mbbiDirectRecord>,
    public virtual InputValueRecord<uint32_t>
{
public:
    explicit MbbiDirectRecord(mbbiDirectRecord *raw) : EpicsInputValueRecord<uint32_t, mbbiDirectRecord>(raw) {}
};
