#pragma once

#include <aiRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class AiRecord :
    public EpicsInputValueRecord<int32_t, aiRecord>,
    public virtual InputValueRecord<int32_t>
{
public:
    explicit AiRecord(aiRecord *raw) : EpicsInputValueRecord<int32_t, aiRecord>(raw) {}
};
