#pragma once

#include <cassert>
#include <cstdint>

#include <waveformRecord.h>

#include "record.hpp"


typedef menuFtype waveform_type;

template <typename T>
struct to_waveform_type;
template <> struct to_waveform_type<float> {
    static const menuFtype value = menuFtypeFLOAT;
};
template <> struct to_waveform_type<double> {
    static const menuFtype value = menuFtypeDOUBLE;
};


class GenericWaveformRecord: public Record {
public:
    GenericWaveformRecord() = default;
    ~GenericWaveformRecord() override = default;

    virtual void read_generic(void *data, size_t count, uint16_t type) = 0;
    virtual waveform_type generic_type() const = 0;
};


template <typename T>
class WaveformRecord : public GenericWaveformRecord {
public:
    WaveformRecord() = default;
    ~WaveformRecord() override = default;

    void read_generic(void *data, size_t count, uint16_t type) override final {
        assert(to_waveform_type<T>::value == type);
        this->read(reinterpret_cast<T*>(data), count);
    }
    waveform_type generic_type() const override final {
        return to_waveform_type<T>::value;
    }

    virtual void read(T *data, size_t size) = 0;
};
