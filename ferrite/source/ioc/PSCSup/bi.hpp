#pragma once

#include <biRecord.h>

#include "value.hpp"

#include <record/value.hpp>

class BiRecord final : public EpicsInputValueRecord<bool, biRecord>, public virtual InputValueRecord<bool> {
public:
    explicit BiRecord(biRecord *raw) : EpicsInputValueRecord<bool, biRecord>(raw) {}

    [[nodiscard]] virtual bool value() const override;
    virtual void set_value(bool value) override;
};
