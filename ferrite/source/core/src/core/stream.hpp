#pragma once

#include <optional>


template <typename T>
class StreamRead {
public:
    //! Try to read at most `len` items into `data` buffer.
    //! @return Number of items read.
    [[nodiscard]] virtual size_t stream_read(T *data, size_t len) = 0;
};

template <typename T>
class StreamWrite {
public:
    //! Try to write at most `len` items from `data` buffer.
    //! @return Number of items written or error.
    [[nodiscard]] virtual size_t stream_write(const T *data, size_t len) = 0;
};

template <typename T>
class StreamReadExact : public virtual StreamRead<T> {
public:
    //! Try to read exactly `len` items into `data` buffer.
    //! @return `true` on success.
    [[nodiscard]] virtual bool stream_read_exact(T *data, size_t len) = 0;
};

template <typename T>
class StreamWriteExact : public virtual StreamWrite<T> {
public:
    //! Try to write exactly `len` items from `data` buffer.
    //! @return `true` on success.
    [[nodiscard]] virtual bool stream_write_exact(const T *data, size_t len) = 0;
};

template <typename T>
class StreamReadFrom {
public:
    //! Read items from given stream into `this`.
    //! @param len Number of items to read. If `nullopt` then read as much as possible.
    [[nodiscard]] virtual size_t stream_read_from(StreamRead<T> &stream, std::optional<size_t> len) = 0;
};

template <typename T>
class StreamWriteInto {
public:
    //! Write items from `this` into given stream.
    //! @param len Number of items to write. If `nullopt` then write as much as possible.
    [[nodiscard]] virtual size_t stream_write_into(StreamWrite<T> &stream, std::optional<size_t> len) = 0;
};
