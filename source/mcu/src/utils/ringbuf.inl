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

#include <hal/assert.h>
#include <hal/math.h>

hal_retcode concat(RB_PREFIX, _init)(RB_STRUCT *self) {
    self->handle = xStreamBufferCreateStatic(_RB_STATIC_DATA_SIZE(RB_CAPACITY), 0, self->data, &self->info);
    hal_assert(self->handle != NULL);
    return HAL_SUCCESS;
}

hal_retcode concat(RB_PREFIX, _deinit)(RB_STRUCT *self) {
    return HAL_SUCCESS;
}


size_t concat(RB_PREFIX, _capacity)(const RB_STRUCT *self) {
    return RB_CAPACITY;
}

size_t concat(RB_PREFIX, _occupied)(const RB_STRUCT *self) {
    size_t len = xStreamBufferBytesAvailable(self->handle);
    hal_assert((len % sizeof(RB_ITEM)) == 0);
    return len / sizeof(RB_ITEM);
}

size_t concat(RB_PREFIX, _vacant)(const RB_STRUCT *self) {
    size_t len = xStreamBufferSpacesAvailable(self->handle);
    hal_assert((len % sizeof(RB_ITEM)) == 0);
    return len / sizeof(RB_ITEM);
}

size_t concat(RB_PREFIX, _read)(RB_STRUCT *self, RB_ITEM *data, size_t max_len) {
    size_t len = xStreamBufferReceive(self->handle, data, max_len * sizeof(RB_ITEM), 0);
    hal_assert((len % sizeof(RB_ITEM)) == 0);
    return len / sizeof(RB_ITEM);
}

size_t concat(RB_PREFIX, _write)(RB_STRUCT *self, const RB_ITEM *data, size_t max_len) {
    size_t len = xStreamBufferSend(self->handle, data, max_len * sizeof(RB_ITEM), 0);
    hal_assert((len % sizeof(RB_ITEM)) == 0);
    return len / sizeof(RB_ITEM);
}

#define TMP_BUF_LEN 16

size_t concat(RB_PREFIX, _overwrite)(RB_STRUCT *self, const RB_ITEM *data, size_t len) {
    hal_assert(len <= concat(RB_PREFIX, _capacity)(self));
    size_t vacant = concat(RB_PREFIX, _vacant)(self);
    size_t extra = 0;
    if (vacant < len) {
        extra = len - vacant;
        RB_ITEM tmp_buf[TMP_BUF_LEN];
        for (size_t i = 0; i < extra / TMP_BUF_LEN; ++i) {
            hal_assert(concat(RB_PREFIX, _read)(self, tmp_buf, TMP_BUF_LEN) == TMP_BUF_LEN);
        }
        size_t extra_rem = extra % TMP_BUF_LEN;
        hal_assert(concat(RB_PREFIX, _read)(self, tmp_buf, extra_rem) == extra_rem);
    }
    hal_assert(concat(RB_PREFIX, _write)(self, data, len) == len);
    return extra;
}

/// Read and discard at most `max_len` points from the ring buffer.
/// @return Number of actually skipped points.
size_t concat(RB_PREFIX, _skip)(RB_STRUCT *self, size_t max_len) {
    size_t len = hal_min(max_len, concat(RB_PREFIX, _occupied)(self));

    RB_ITEM tmp_buf[TMP_BUF_LEN];
    for (size_t i = 0; i < len / TMP_BUF_LEN; ++i) {
        hal_assert(concat(RB_PREFIX, _read)(self, tmp_buf, TMP_BUF_LEN) == TMP_BUF_LEN);
    }
    size_t rem = len % TMP_BUF_LEN;
    hal_assert(concat(RB_PREFIX, _read)(self, tmp_buf, rem) == rem);

    return len;
}

#undef TMP_BUF_LEN
