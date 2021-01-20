#pragma once

#include <memory>

#include <record_waveform.hpp>

[[nodiscard]]
std::unique_ptr<WaveformHandler> framework_record_init_waveform(WaveformRecord &record);
