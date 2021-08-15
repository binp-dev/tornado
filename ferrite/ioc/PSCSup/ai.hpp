#pragma once

#include <aiRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class AiRecord :
    public EpicsInputValueRecord<uint32_t, aiRecord>,
    public virtual InputValueRecord<uint32_t>
{
public:
    explicit AiRecord(aiRecord *raw) : EpicsInputValueRecord<uint32_t, aiRecord>(raw) {}
};
