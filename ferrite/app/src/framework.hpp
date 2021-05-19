#pragma once

#include <memory>

#include <record/waveform.hpp>

void framework_init_device();

[[nodiscard]]
std::unique_ptr<WaveformHandler> framework_record_init_waveform(WaveformRecord &record);
