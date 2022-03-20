#include "ringbuf.h"

#include <hal/assert.h>
#include <hal/math.h>

hal_retcode rb_init(RingBuffer *self, size_t len) {
    self->stream_buffer = xStreamBufferCreate(len * sizeof(point_t), 0);
    if (self->stream_buffer == NULL) {
        return HAL_BAD_ALLOC;
    }
    self->capacity = len;
    return HAL_SUCCESS;
}

hal_retcode rb_deinit(RingBuffer *self) {
    vStreamBufferDelete(self->stream_buffer);
    return HAL_SUCCESS;
}


size_t rb_capacity(const RingBuffer *self) {
    return self->capacity;
}

size_t rb_occupied(const RingBuffer *self) {
    size_t len = xStreamBufferBytesAvailable(self->stream_buffer);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_vacant(const RingBuffer *self) {
    size_t len = xStreamBufferSpacesAvailable(self->stream_buffer);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_read(RingBuffer *self, point_t *data, size_t max_len) {
    size_t len = xStreamBufferReceive(self->stream_buffer, data, max_len * sizeof(point_t), 0);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_write(RingBuffer *self, const point_t *data, size_t max_len) {
    size_t len = xStreamBufferSend(self->stream_buffer, data, max_len * sizeof(point_t), 0);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

#define TMP_BUF_LEN 16

size_t rb_overwrite(RingBuffer *self, const point_t *data, size_t len) {
    hal_assert(len <= self->capacity);
    size_t vacant = rb_vacant(self);
    size_t extra = 0;
    if (vacant < len) {
        extra = len - vacant;
        point_t tmp_buf[TMP_BUF_LEN];
        for (size_t i = 0; i < extra / TMP_BUF_LEN; ++i) {
            hal_assert(rb_read(self, tmp_buf, TMP_BUF_LEN) == TMP_BUF_LEN);
        }
        size_t extra_rem = extra % TMP_BUF_LEN;
        hal_assert(rb_read(self, tmp_buf, extra_rem) == extra_rem);
    }
    hal_assert(rb_write(self, data, len) == len);
    return extra;
}

/// Read and discard at most `max_len` points from the ring buffer.
/// @return Number of actually skipped points.
size_t rb_skip(RingBuffer *self, size_t max_len) {
    size_t len = hal_min(max_len, rb_occupied(self));

    point_t tmp_buf[TMP_BUF_LEN];
    for (size_t i = 0; i < len / TMP_BUF_LEN; ++i) {
        hal_assert(rb_read(self, tmp_buf, TMP_BUF_LEN) == TMP_BUF_LEN);
    }
    size_t rem = len % TMP_BUF_LEN;
    hal_assert(rb_read(self, tmp_buf, rem) == rem);

    return len;
}
