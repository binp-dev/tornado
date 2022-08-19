#pragma once

#include <boRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class BoRecord final : public EpicsOutputValueRecord<bool, boRecord>, public virtual OutputValueRecord<bool> {
public:
    inline explicit BoRecord(boRecord *raw) : EpicsOutputValueRecord<bool, boRecord>(raw) {}

    [[nodiscard]] virtual bool value() const override;
};
