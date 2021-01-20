#include "waveform.hpp"


const waveformRecord *WaveformRecord::raw() const {
    return (const waveformRecord *)Record::raw();
}
waveformRecord *WaveformRecord::raw() {
    return (waveformRecord *)Record::raw();
}

waveform_type WaveformRecord::waveform_data_type() const {
    return static_cast<waveform_type>(raw()->ftvl);
}

const void *WaveformRecord::waveform_raw_data() const {
    return raw()->bptr;
}
void *WaveformRecord::waveform_raw_data() {
    return raw()->bptr;
}

size_t WaveformRecord::waveform_max_length() const {
    return raw()->nelm;
}
size_t WaveformRecord::waveform_length() const {
    return raw()->nord;
}
const WaveformHandler &WaveformRecord::handler() const {
    return *(const WaveformHandler *)private_data();
}
WaveformHandler &WaveformRecord::handler() {
    return *(WaveformHandler *)private_data();
}
