#pragma once

#include <stdlib.h>

#include <FreeRTOS.h>
#include <stream_buffer.h>

#include <hal/defs.h>

#include <common/config.h>

/// Ring buffer structure.
typedef struct {
    StreamBufferHandle_t stream_buffer;
    size_t capacity;
} RingBuffer;


/// Initialize ring buffer aloocating memory.
/// @param len Max number of points that could be stored within.
hal_retcode rb_init(RingBuffer *self, size_t len);

/// Deinitialize previously initialized ring buffer deallocating its memory.
hal_retcode rb_deinit(RingBuffer *self);


/// Total capacity (in points) of the ring buffer.
size_t rb_capacity(const RingBuffer *self);

/// Numbers of points stored in the ring buffer.
size_t rb_occupied(const RingBuffer *self);

/// Number of additionaly points that could be stored in free space of the ring buffer.
size_t rb_vacant(const RingBuffer *self);


/// Read at most `max_len` points from the ring buffer into `data`.
/// @return Number of actially read points.
size_t rb_read(RingBuffer *self, point_t *data, size_t max_len);

/// Write at most `max_len` points into the ring buffer from `data`.
/// @return Number of actially written points.
size_t rb_write(RingBuffer *self, const point_t *data, size_t max_len);

/// Write `len` points into the ring buffer from `data` overwriting oldest points if there is unsufficient free space.
/// @return Number of overwritten oldest points.
size_t rb_overwrite(RingBuffer *self, const point_t *data, size_t len);
