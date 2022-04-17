#include "stream.hpp"

namespace core {

inline namespace {

struct StreamReadArrayWrapper final : ReadArray<uint8_t> {
    io::StreamRead &stream;
    std::optional<io::Error> error = std::nullopt;

    StreamReadArrayWrapper(io::StreamRead &s) : stream(s) {}

    [[nodiscard]] size_t read_array(std::span<uint8_t> data) override {
        auto res = stream.stream_read(data);
        if (res.is_ok()) {
            return res.ok();
        } else {
            error = res.unwrap_err();
            return 0;
        }
    }
};

struct StreamWriteArrayWrapper final : WriteArray<uint8_t> {
    io::StreamWrite &stream;
    std::optional<io::Error> error = std::nullopt;

    StreamWriteArrayWrapper(io::StreamWrite &s) : stream(s) {}

    [[nodiscard]] size_t write_array(std::span<const uint8_t> data) override {
        auto res = stream.stream_write(data);
        if (res.is_ok()) {
            return res.ok();
        } else {
            error = res.unwrap_err();
            return 0;
        }
    }
};


} // namespace


Result<size_t, io::Error> ReadArray<uint8_t>::stream_read(std::span<uint8_t> data) {
    return Ok(this->read_array(data));
}

Result<std::monostate, io::Error> ReadArrayExact<uint8_t>::stream_read_exact(std::span<uint8_t> data) {
    if (this->read_array_exact(data)) {
        return Ok(std::monostate{});
    } else {
        return Err(io::Error{io::ErrorKind::UnexpectedEof});
    }
}

Result<size_t, io::Error> WriteArray<uint8_t>::stream_write(std::span<const uint8_t> data) {
    return Ok(this->write_array(data));
}

Result<std::monostate, io::Error> WriteArrayExact<uint8_t>::stream_write_exact(std::span<const uint8_t> data) {
    if (this->write_array_exact(data)) {
        return Ok(std::monostate{});
    } else {
        return Err(io::Error{io::ErrorKind::UnexpectedEof});
    }
}

Result<size_t, io::Error> WriteArrayFrom<uint8_t>::write_from_stream(io::StreamRead &stream, std::optional<size_t> len_opt) {
    StreamReadArrayWrapper wrapper{stream};
    size_t read_len = this->write_array_from(wrapper, len_opt);
    if (wrapper.error.has_value()) {
        return Err(std::move(wrapper.error.value()));
    } else {
        return Ok(read_len);
    }
}

Result<size_t, io::Error> ReadArrayInto<uint8_t>::read_into_stream(io::StreamWrite &stream, std::optional<size_t> len_opt) {
    StreamWriteArrayWrapper wrapper{stream};
    size_t write_len = this->read_array_into(wrapper, len_opt);
    if (wrapper.error.has_value()) {
        return Err(std::move(wrapper.error.value()));
    } else {
        return Ok(write_len);
    }
}

} // namespace core
