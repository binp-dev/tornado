#pragma once

#include <cstdint>
#include <utility>

#include <core/assert.hpp>
#include <record/array.hpp>

#include "base.hpp"
#include "types.hpp"

#include <waveformRecord.h>

template <typename T>
class WaveformHandler;

template <typename T>
class WaveformRecord final :
    public virtual InputArrayRecord<T>,
    public EpicsRecord<waveformRecord, WaveformHandler<T>>
{
public:
    explicit WaveformRecord(waveformRecord &raw) : EpicsRecord(raw) {
        assert_eq(epics_scalar_enum<T>(), data_type());
    }

private:
    menuFtype data_type() const {
        return static_cast<menuFtype>(raw().ftvl);
    }

    const void *raw_data() const {
        return raw().bptr;
    }
    void *raw_data() {
        return raw().bptr;
    }

public:
    virtual const T *data() const override {
        return (const T *)raw_data();
    }

    virtual size_t max_length() const override {
        return raw().nelm;
    }
    virtual size_t length() const override {
        return raw().nord;
    }
};

template <typename T>
class WaveformHandler : public Handler {
public:
    virtual void read(InputArrayRecord<T> &record) = 0;
};
