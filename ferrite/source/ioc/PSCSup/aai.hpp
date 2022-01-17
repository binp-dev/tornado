#pragma once

#include <cstdint>
#include <utility>

#include <core/assert.hpp>
#include <record/array.hpp>

#include <aaiRecord.h>

#include "array.hpp"

template <typename T>
class AaiRecord final : public EpicsInputArrayRecord<T, aaiRecord> {
public:
    explicit AaiRecord(aaiRecord *raw) : EpicsInputArrayRecord<T, aaiRecord>(raw) {}
};
