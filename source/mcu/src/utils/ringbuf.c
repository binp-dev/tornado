#include "ringbuf.h"

#include <hal/assert.h>

hal_retcode rb_init(RingBuffer *rb, size_t len) {
    rb->stream_buffer = xStreamBufferCreate(len * sizeof(point_t), 0);
    if (rb->stream_buffer == NULL) {
        return HAL_BAD_ALLOC;
    }
    rb->capacity = len;
    return HAL_SUCCESS;
}

hal_retcode rb_deinit(RingBuffer *rb) {
    xStreamBufferDelete(rb->stream_buffer);
    return HAL_SUCCESS;
}


size_t rb_capacity(const RingBuffer *rb) {
    return rb->capacity;
}

size_t rb_occupied(const RingBuffer *rb) {
    size_t len = xStreamBufferBytesAvailable(rb->stream_buffer);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_vacant(const RingBuffer *rb) {
    size_t len = xStreamBufferSpacesAvailable(rb->stream_buffer);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_read(RingBuffer *rb, point_t *data, size_t max_len) {
    size_t len = xStreamBufferReceive(rb->stream_buffer, data, max_len * sizeof(point_t), 0);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}

size_t rb_write(RingBuffer *rb, const point_t *data, size_t max_len) {
    size_t len = xStreamBufferSend(rb->stream_buffer, data, max_len * sizeof(point_t), 0);
    hal_assert((len % sizeof(point_t)) == 0);
    return len / sizeof(point_t);
}
