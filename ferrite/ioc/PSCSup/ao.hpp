#pragma once

#include <aoRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class AoRecord :
    public EpicsOutputValueRecord<uint32_t, aoRecord>,
    public virtual OutputValueRecord<uint32_t>
{
public:
    explicit AoRecord(aoRecord *raw) : EpicsOutputValueRecord<uint32_t, aoRecord>(raw) {}
};
