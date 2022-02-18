#include "vec_deque.hpp"

Result<size_t, io::Error> VecDequeStream::read(uint8_t *data, size_t len) {
    /// Copy data from queue.
    auto [left, right] = queue.as_slices();
    size_t left_len = std::min(left.size(), len);
    memcpy(data, left.data(), left_len);
    size_t right_len = std::min(right.size(), len - left_len);
    memcpy(data + left_len, right.data(), right_len);

    size_t read_len = left_len + right_len;
    assert_eq(queue.skip_front(read_len), read_len);
    return Ok(read_len);
}

Result<std::monostate, io::Error> VecDequeStream::read_exact(uint8_t *data, size_t len) {
    if (len <= queue.size()) {
        assert_eq(read(data, len).unwrap(), len);
        return Ok(std::monostate());
    } else {
        return Err(io::Error{io::ErrorKind::UnexpectedEof});
    }
}

Result<size_t, io::Error> VecDequeStream::write(const uint8_t *data, size_t len) {
    /// Reserve enough space for new elements.
    size_t new_mod = std::max(queue.mod(), size_t(2));
    while (new_mod < queue.size() + len + 1) {
        new_mod = 2 * new_mod;
    }
    queue.reserve_mod(new_mod);

    /// Copy data to queue.
    auto [left, right] = queue.free_space_as_slices();
    size_t left_len = std::min(left.size(), len);
    memcpy(left.data(), data, left_len);
    size_t right_len = std::min(right.size(), len - left_len);
    memcpy(right.data(), data + left_len, right_len);

    assert_eq(left_len + right_len, len);
    queue.expand_back(len);
    return Ok(len);
}

Result<std::monostate, io::Error> VecDequeStream::write_exact(const uint8_t *data, size_t len) {
    assert_eq(write(data, len).unwrap(), len);
    return Ok(std::monostate{});
}
