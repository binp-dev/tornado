#include "vec_deque.hpp"

#include <core/numeric.hpp>

Result<size_t, io::Error> VecDeque<uint8_t>::read(uint8_t *data, size_t len) {
    // Copy data from this->
    auto [left, right] = this->as_slices();
    size_t left_len = std::min(left.size(), len);
    memcpy(data, left.data(), left_len);
    size_t right_len = std::min(right.size(), len - left_len);
    memcpy(data + left_len, right.data(), right_len);

    size_t read_len = left_len + right_len;
    assert_eq(this->skip_front(read_len), read_len);
    return Ok(read_len);
}

Result<std::monostate, io::Error> VecDeque<uint8_t>::read_exact(uint8_t *data, size_t len) {
    if (len <= this->size()) {
        assert_eq(read(data, len).unwrap(), len);
        return Ok(std::monostate());
    } else {
        return Err(io::Error{io::ErrorKind::UnexpectedEof});
    }
}

Result<size_t, io::Error> VecDeque<uint8_t>::write(const uint8_t *data, size_t len) {
    // Reserve enough space for new elements.
    this->grow_to_free(len);

    // Copy data to queue.
    auto [left, right] = this->free_space_as_slices();
    size_t left_len = std::min(left.size(), len);
    memcpy(left.data(), data, left_len);
    size_t right_len = std::min(right.size(), len - left_len);
    memcpy(right.data(), data + left_len, right_len);

    assert_eq(left_len + right_len, len);
    this->expand_back(len);
    return Ok(len);
}

Result<std::monostate, io::Error> VecDeque<uint8_t>::write_exact(const uint8_t *data, size_t len) {
    assert_eq(write(data, len).unwrap(), len);
    return Ok(std::monostate{});
}

Result<size_t, io::Error> VecDeque<uint8_t>::read_from(io::Read &stream, std::optional<size_t> len_opt) {
    if (len_opt.has_value()) {
        size_t len = len_opt.value();

        // Reserve enough space for new elements.
        this->grow_to_free(len);

        // Read first slice.
        auto [left, right] = this->free_space_as_slices();
        size_t left_len = std::min(left.size(), len);
        auto left_res = stream.read(reinterpret_cast<uint8_t *>(left.data()), left_len);
        if (left_res.is_ok()) {
            this->expand_back(left_res.ok());
        }
        if (left_res.is_err() || left_res.ok() < left_len) {
            return left_res;
        }

        // Read second slice.
        size_t right_len = std::min(right.size(), len - left_len);
        auto right_res = stream.read(reinterpret_cast<uint8_t *>(right.data()), right_len);
        if (right_res.is_err()) {
            return Ok(left_len);
        } else {
            this->expand_back(right_res.ok());
            return Ok(left_len + right_res.ok());
        }
    } else {
        // Read infinitely until stream ends.
        size_t total = 0;
        for (;;) {
            size_t free = this->capacity() - this->size();
            if (free > 0) {
                auto res = read_from(stream, free);
                if (res.is_err()) {
                    return res;
                }
                total += res.ok();
                if (res.ok() < free) {
                    return Ok(total);
                }
            }
            this->grow();
        }
    }
}

Result<size_t, io::Error> VecDeque<uint8_t>::write_to(io::Write &stream, std::optional<size_t> len_opt) {
    size_t len = len_opt.value_or(this->size());
    auto [left, right] = this->as_slices();

    // Write first slice.
    size_t left_len = std::min(left.size(), len);
    auto left_res = stream.write(left.data(), left_len);
    if (left_res.is_ok()) {
        assert_eq(this->skip_front(left_res.ok()), left_res.ok());
    }
    if (left_res.is_err() || left_res.ok() < left_len) {
        return left_res;
    }

    // Write second slice.
    size_t right_len = std::min(right.size(), len - left_len);
    auto right_res = stream.write(right.data(), right_len);
    if (right_res.is_err()) {
        return Ok(left_len);
    } else {
        assert_eq(this->skip_front(right_res.ok()), right_res.ok());
        return Ok(left_len + right_res.ok());
    }
}
