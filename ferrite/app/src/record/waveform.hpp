#pragma once

#include <cstdint>
#include <utility>

#include <waveformRecord.h>
#include <menuFtype.h>

#include "base.hpp"


typedef menuFtype waveform_type;

template <typename T>
struct to_waveform_type;
template <> struct to_waveform_type<float> {
    static const menuFtype value = menuFtypeFLOAT;
};
template <> struct to_waveform_type<double> {
    static const menuFtype value = menuFtypeDOUBLE;
};

class WaveformHandler;

class WaveformRecord final : public Record {
public:
    inline explicit WaveformRecord(waveformRecord *raw) : Record((dbCommon *)raw) {}
    virtual ~WaveformRecord() override = default;

private:
    const waveformRecord *raw() const;
    waveformRecord *raw();

    waveform_type waveform_data_type() const;

    const void *waveform_raw_data() const;
    void *waveform_raw_data();

public:
    template <typename T>
    const T *waveform_data() const {
        assert(to_waveform_type<T>::value == waveform_data_type());
        return (const T *)waveform_raw_data();
    }
    template <typename T>
    T *waveform_data() {
        assert(to_waveform_type<T>::value == waveform_data_type());
        return (T *)waveform_raw_data();
    }
    template <typename T>
    std::pair<const T *, size_t> waveform_slice() const {
        return std::pair(waveform_data<T>(), waveform_length());
    }
    template <typename T>
    std::pair<T *, size_t> waveform_slice() {
        return std::pair(waveform_data<T>(), waveform_length());
    }

    size_t waveform_max_length() const;
    size_t waveform_length() const;
    const WaveformHandler &handler() const;
    WaveformHandler &handler();
};

class WaveformHandler : public Handler {
public:
    WaveformHandler() = default;
    virtual ~WaveformHandler() override = default;

    virtual void read(WaveformRecord &record) = 0;
};
