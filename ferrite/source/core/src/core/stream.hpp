#pragma once

#include <optional>
#include <type_traits>

#include "io.hpp"


template <typename T>
class ReadArray;

template <typename T>
class WriteArray;


namespace stream_impl {

template <typename T>
class ReadArray {
public:
    static_assert(std::is_trivial_v<T>);

    //! Try to read at most `len` items into `data` buffer.
    //! @return Number of items read.
    [[nodiscard]] virtual size_t read_array(T *data, size_t len) = 0;
};

template <typename T>
class WriteArray {
public:
    static_assert(std::is_trivial_v<T>);

    //! Try to write at most `len` items from `data` buffer.
    //! @return Number of items written or error.
    [[nodiscard]] virtual size_t write_array(const T *data, size_t len) = 0;
};

template <typename T>
class ReadArrayExact : public virtual ReadArray<T> {
public:
    //! Try to read exactly `len` items into `data` buffer.
    //! @return `true` on success.
    [[nodiscard]] virtual bool read_array_exact(T *data, size_t len) = 0;

    [[nodiscard]] size_t read_array(T *data, size_t len) override {
        if (read_array_exact(data, len)) {
            return len;
        } else {
            return 0;
        }
    }
};

template <typename T>
class WriteArrayExact : public virtual WriteArray<T> {
public:
    //! Try to write exactly `len` items from `data` buffer.
    //! @return `true` on success.
    [[nodiscard]] virtual bool write_array_exact(const T *data, size_t len) = 0;

    [[nodiscard]] size_t write_array(const T *data, size_t len) override {
        if (write_array_exact(data, len)) {
            return len;
        } else {
            return 0;
        }
    }
};

template <typename T>
class WriteArrayFrom {
public:
    //! Read items from given stream into `this`.
    //! @param len Number of items to read. If `nullopt` then read as much as possible.
    virtual size_t write_array_from(::ReadArray<T> &stream, std::optional<size_t> len) = 0;
};

template <typename T>
class ReadArrayInto {
public:
    //! Write items from `this` into given stream.
    //! @param len Number of items to write. If `nullopt` then write as much as possible.
    virtual size_t read_array_into(::WriteArray<T> &stream, std::optional<size_t> len) = 0;
};

} // namespace stream_impl


template <typename T>
class ReadArray : public virtual stream_impl::ReadArray<T> {};

template <typename T>
class WriteArray : public virtual stream_impl::WriteArray<T> {};

template <typename T>
class ReadArrayExact : public virtual ReadArray<T>, public virtual stream_impl::ReadArrayExact<T> {};

template <typename T>
class WriteArrayExact : public virtual WriteArray<T>, public virtual stream_impl::WriteArrayExact<T> {};

template <typename T>
class WriteArrayFrom : public virtual stream_impl::WriteArrayFrom<T> {};

template <typename T>
class ReadArrayInto : public virtual stream_impl::ReadArrayInto<T> {};


template <>
class ReadArray<uint8_t> :
    public virtual stream_impl::ReadArray<uint8_t>,
    public virtual io::StreamRead //
{
public:
    Result<size_t, io::Error> stream_read(uint8_t *data, size_t len) override;
};

template <>
class WriteArray<uint8_t> :
    public virtual stream_impl::WriteArray<uint8_t>,
    public virtual io::StreamWrite //
{
public:
    Result<size_t, io::Error> stream_write(const uint8_t *data, size_t len) override;
};

template <>
class ReadArrayExact<uint8_t> :
    public virtual ReadArray<uint8_t>,
    public virtual stream_impl::ReadArrayExact<uint8_t>,
    public virtual io::StreamReadExact //
{
public:
    Result<std::monostate, io::Error> stream_read_exact(uint8_t *data, size_t len) override;
};

template <>
class WriteArrayExact<uint8_t> :
    public virtual WriteArray<uint8_t>,
    public virtual stream_impl::WriteArrayExact<uint8_t>,
    public virtual io::StreamWriteExact //
{
public:
    Result<std::monostate, io::Error> stream_write_exact(const uint8_t *data, size_t len) override;
};

template <>
class WriteArrayFrom<uint8_t> :
    public virtual stream_impl::WriteArrayFrom<uint8_t>,
    public virtual io::WriteFromStream //
{
public:
    Result<size_t, io::Error> write_from_stream(io::StreamRead &stream, std::optional<size_t> len) override;
};

template <>
class ReadArrayInto<uint8_t> :
    public virtual stream_impl::ReadArrayInto<uint8_t>,
    public virtual io::ReadIntoStream //
{
public:
    Result<size_t, io::Error> read_into_stream(io::StreamWrite &stream, std::optional<size_t> len) override;
};