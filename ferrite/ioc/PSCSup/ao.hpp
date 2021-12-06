#pragma once

#include <aoRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class AoRecord :
    public EpicsOutputValueRecord<int32_t, aoRecord>,
    public virtual OutputValueRecord<int32_t>
{
public:
    explicit AoRecord(aoRecord *raw) : EpicsOutputValueRecord<int32_t, aoRecord>(raw) {}
};
