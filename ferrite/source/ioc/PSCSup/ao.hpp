#pragma once

#include <aoRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class AoRecord final : public EpicsOutputValueRecord<int32_t, aoRecord>, public virtual OutputValueRecord<int32_t> {
public:
    inline explicit AoRecord(aoRecord *raw) : EpicsOutputValueRecord<int32_t, aoRecord>(raw) {}

    [[nodiscard]] virtual int32_t value() const override;
};
