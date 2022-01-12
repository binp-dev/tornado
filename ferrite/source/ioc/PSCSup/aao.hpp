#pragma once

#include <cstdint>
#include <utility>

#include <core/assert.hpp>
#include <record/array.hpp>

#include <aaoRecord.h>

#include "array.hpp"

template <typename T>
class AaoRecord final : public EpicsOutputArrayRecord<T, aaoRecord> {
public:
    explicit AaoRecord(aaoRecord *raw) : EpicsOutputArrayRecord<T, aaoRecord>(raw) {}
};
