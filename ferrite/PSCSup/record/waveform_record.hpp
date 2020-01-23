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

class WaveformHandler;

class WaveformRecord : public Record {
public:
    WaveformRecord(waveformRecord *raw) : Record((dbCommon *)raw) {}
    ~WaveformRecord() override = default;

    const waveformRecord *raw() const {
        return (const waveformRecord *)Record::raw();
    }
    waveformRecord *raw() {
        return (waveformRecord *)Record::raw();
    }

    waveform_type waveform_data_type() const {
        return static_cast<waveform_type>(raw()->ftvl);
    }

    const void *waveform_raw_data() const {
        return raw()->bptr;
    }
    void *waveform_raw_data() {
        return raw()->bptr;
    }
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

    size_t waveform_max_length() const {
        return raw()->nelm;
    }
    size_t waveform_length() const {
        return raw()->nord;
    }
    const WaveformHandler &handler() const {
        return *(const WaveformHandler *)private_data();
    }
    WaveformHandler &handler() {
        return *(WaveformHandler *)private_data();
    }
};

class WaveformHandler : public Handler {
public:
    WaveformHandler() = default;
    ~WaveformHandler() override = default;

    virtual void read(WaveformRecord &record) = 0;
};
