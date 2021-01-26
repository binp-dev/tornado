#pragma once

#include <memory>

#include <record/waveform.hpp>

[[nodiscard]]
std::unique_ptr<WaveformHandler> framework_record_init_waveform(WaveformRecord &record);
