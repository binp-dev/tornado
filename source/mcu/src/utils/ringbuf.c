#include "ringbuf.h"

#include <hal/assert.h>
#include <hal/math.h>

hal_retcode rb_init(RingBuffer *self, uint8_t *static_data, size_t len) {
    self->handle = xStreamBufferCreateStatic(_RB_STATIC_DATA_SIZE(len), 0, static_data, &self->info);
    hal_assert(self->handle != NULL);
    self->capacity = len;
    return HAL_SUCCESS;
}

hal_retcode rb_deinit(RingBuffer *self) {
    return HAL_SUCCESS;
}


size_t rb_capacity(const RingBuffer *self) {
    return self->capacity;
}

size_t rb_occupied(const RingBuffer *self) {
    size_t len = xStreamBufferBytesAvailable(self->handle);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_vacant(const RingBuffer *self) {
    size_t len = xStreamBufferSpacesAvailable(self->handle);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_read(RingBuffer *self, point_t *data, size_t max_len) {
    size_t len = xStreamBufferReceive(self->handle, data, max_len * sizeof(point_t), 0);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_write(RingBuffer *self, const point_t *data, size_t max_len) {
    size_t len = xStreamBufferSend(self->handle, data, max_len * sizeof(point_t), 0);
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
