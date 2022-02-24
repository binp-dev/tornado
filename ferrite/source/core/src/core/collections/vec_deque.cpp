#include "vec_deque.hpp"

Result<size_t, io::Error> VecDeque<uint8_t>::read(uint8_t *data, size_t len) {
    return Ok(this->stream_read(data, len));
}

Result<std::monostate, io::Error> VecDeque<uint8_t>::read_exact(uint8_t *data, size_t len) {
    if (this->stream_read_exact(data, len)) {
        return Ok(std::monostate{});
    } else {
        return Err(io::Error{io::ErrorKind::UnexpectedEof});
    }
}

Result<size_t, io::Error> VecDeque<uint8_t>::write(const uint8_t *data, size_t len) {
    return Ok(this->stream_write(data, len));
}

Result<std::monostate, io::Error> VecDeque<uint8_t>::write_exact(const uint8_t *data, size_t len) {
    if (this->stream_write_exact(data, len)) {
        return Ok(std::monostate{});
    } else {
        return Err(io::Error{io::ErrorKind::UnexpectedEof});
    }
}

Result<size_t, io::Error> VecDeque<uint8_t>::read_from(io::Read &stream, std::optional<size_t> len_opt) {
    io::StreamReadWrapper wrapper{stream};
    size_t read_len = this->stream_read_from(wrapper, len_opt);
    if (wrapper.error.has_value()) {
        return Err(std::move(wrapper.error.value()));
    } else {
        return Ok(read_len);
    }
}

Result<size_t, io::Error> VecDeque<uint8_t>::write_into(io::Write &stream, std::optional<size_t> len_opt) {
    io::StreamWriteWrapper wrapper{stream};
    size_t write_len = this->stream_write_into(wrapper, len_opt);
    if (wrapper.error.has_value()) {
        return Err(std::move(wrapper.error.value()));
    } else {
        return Ok(write_len);
    }
}

Result<size_t, io::Error> VecDequeView<uint8_t>::read(uint8_t *data, size_t len) {
    return Ok(this->stream_read(data, len));
}

Result<std::monostate, io::Error> VecDequeView<uint8_t>::read_exact(uint8_t *data, size_t len) {
    if (this->stream_read_exact(data, len)) {
        return Ok(std::monostate{});
    } else {
        return Err(io::Error{io::ErrorKind::UnexpectedEof});
    }
}

Result<size_t, io::Error> VecDequeView<uint8_t>::write_into(io::Write &stream, std::optional<size_t> len_opt) {
    io::StreamWriteWrapper wrapper{stream};
    size_t write_len = this->stream_write_into(wrapper, len_opt);
    if (wrapper.error.has_value()) {
        return Err(std::move(wrapper.error.value()));
    } else {
        return Ok(write_len);
    }
}
