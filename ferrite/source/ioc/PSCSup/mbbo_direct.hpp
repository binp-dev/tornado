#pragma once

#include <mbboDirectRecord.h>

#include "value.hpp"

#include <record/value.hpp>

// TODO: Fix possible issues: https://epics-base.github.io/epics-base/mbboDirectRecord.html
class MbboDirectRecord :
    public EpicsOutputValueRecord<uint32_t, mbboDirectRecord>,
    public virtual OutputValueRecord<uint32_t>
{
public:
    explicit MbboDirectRecord(mbboDirectRecord *raw) : EpicsOutputValueRecord<uint32_t, mbboDirectRecord>(raw) {}
};
