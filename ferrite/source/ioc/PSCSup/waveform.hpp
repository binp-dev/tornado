#pragma once

#include <cstdint>
#include <utility>

#include <core/assert.hpp>
#include <record/array.hpp>

#include <waveformRecord.h>

#include "array.hpp"

template <typename T>
class WaveformRecord final : public virtual InputArrayRecord<T>, public EpicsInputArrayRecord<T, waveformRecord> {
public:
    explicit WaveformRecord(waveformRecord *raw) : EpicsInputArrayRecord<T, waveformRecord>(raw) {}
};
