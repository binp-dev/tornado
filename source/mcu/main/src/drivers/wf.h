#pragma once

#include <stdlib.h>
#include <stdint.h>

extern const size_t wf_max_offset;

/// @note `offset` and `align` must be the multiple of this value.
extern const size_t wf_offset_align;

/// @brief Get address of waveform memory fragment.
volatile uint8_t *wf_addr(size_t offset);

/// @brief Flush external writes to waveform memory fragment.
///        Should be called before reading.
void wf_acquire(uint8_t *addr, size_t len);
/// @brief Flush local writes to waveform memory fragment.
///        Should be called after writing.
void wf_release(uint8_t *addr, size_t len);
