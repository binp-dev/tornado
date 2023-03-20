#ifndef RB_STRUCT
#error "RB_STRUCT identifier must be defined"
#endif // RB_STRUCT
#ifndef RB_PREFIX
#error "RB_PREFIX identifier must be defined"
#endif // RB_PREFIX
#ifndef RB_ITEM
#error "RB_ITEM type must be defined"
#endif // RB_ITEM
#ifndef RB_CAPACITY
#error "RB_CAPACITY number must be defined"
#endif // RB_CAPACITY

#include <stdlib.h>

#include "macros.h"

#include <FreeRTOS.h>
#include <stream_buffer.h>

#include <hal/defs.h>

#include <common/config.h>

#define _RB_STATIC_DATA_SIZE(len) (sizeof(RB_ITEM) * (len) + 1)

/// Ring buffer structure.
typedef struct {
    StaticStreamBuffer_t info;
    StreamBufferHandle_t handle;
    uint8_t data[_RB_STATIC_DATA_SIZE(RB_CAPACITY)];
} RB_STRUCT;


/// Initialize ring buffer aloocating memory.
hal_retcode concat(RB_PREFIX, _init)(RB_STRUCT *self);

/// Deinitialize previously initialized ring buffer deallocating its memory.
hal_retcode concat(RB_PREFIX, _deinit)(RB_STRUCT *self);


/// Total capacity (in points) of the ring buffer.
size_t concat(RB_PREFIX, _capacity)(const RB_STRUCT *self);

/// Numbers of points stored in the ring buffer.
size_t concat(RB_PREFIX, _occupied)(const RB_STRUCT *self);

/// Number of additionaly points that could be stored in free space of the ring buffer.
size_t concat(RB_PREFIX, _vacant)(const RB_STRUCT *self);


/// Read at most `max_len` points from the ring buffer into `data`.
/// @return Number of actually read points.
size_t concat(RB_PREFIX, _read)(RB_STRUCT *self, RB_ITEM *data, size_t max_len);

/// Write at most `max_len` points into the ring buffer from `data`.
/// @return Number of actually written points.
size_t concat(RB_PREFIX, _write)(RB_STRUCT *self, const RB_ITEM *data, size_t max_len);

/// Write `len` points into the ring buffer from `data` overwriting oldest points if there is unsufficient free space.
/// NOTE: This function is both reader and writer at once.
///       You must put it into critical section of there are concurring reading or writing.
/// @return Number of overwritten oldest points.
size_t concat(RB_PREFIX, _overwrite)(RB_STRUCT *self, const RB_ITEM *data, size_t len);

/// Read and discard at most `max_len` points from the ring buffer.
/// @return Number of actually skipped points.
size_t concat(RB_PREFIX, _skip)(RB_STRUCT *self, size_t max_len);
