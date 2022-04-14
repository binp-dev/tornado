#pragma once

#include <optional>
#include <span>
#include <type_traits>
#include <concepts>

#include "io.hpp"

namespace core {

template <typename T>
class ReadArray;

template <typename T>
class WriteArray;


namespace _impl {

template <typename T>
    requires std::is_trivial_v<T>
class BasicReadArray {
public:
    //! Try to read at most `len` items into `data` buffer.
    //! @return Number of items read.
    [[nodiscard]] virtual size_t read_array(std::span<T> data) = 0;
};

template <typename T>
    requires std::is_trivial_v<T>
class BasicWriteArray {
public:
    //! Try to write at most `len` items from `data` buffer.
    //! @return Number of items written or error.
    [[nodiscard]] virtual size_t write_array(std::span<const T> data) = 0;
};

template <typename T>
class BasicReadArrayExact : public virtual BasicReadArray<T> {
public:
    //! Try to read exactly `len` items into `data` buffer.
    //! @return `true` on success.
    [[nodiscard]] virtual bool read_array_exact(std::span<T> data) = 0;

    [[nodiscard]] size_t read_array(std::span<T> data) override {
        if (read_array_exact(data)) {
            return data.size();
        } else {
            return 0;
        }
    }
};

template <typename T>
class BasicWriteArrayExact : public virtual BasicWriteArray<T> {
public:
    //! Try to write exactly `len` items from `data` buffer.
    //! @return `true` on success.
    [[nodiscard]] virtual bool write_array_exact(std::span<const T> data) = 0;

    [[nodiscard]] size_t write_array(std::span<const T> data) override {
        if (write_array_exact(data)) {
            return data.size();
        } else {
            return 0;
        }
    }
};

template <typename T>
class BasicWriteArrayFrom {
public:
    //! Read items from given stream into `this`.
    //! @param len Number of items to read. If `nullopt` then read as much as possible.
    virtual size_t write_array_from(ReadArray<T> &stream, std::optional<size_t> len) = 0;
};

template <typename T>
class BasicReadArrayInto {
public:
    //! Write items from `this` into given stream.
    //! @param len Number of items to write. If `nullopt` then write as much as possible.
    virtual size_t read_array_into(WriteArray<T> &stream, std::optional<size_t> len) = 0;
};

} // namespace _impl


template <typename T>
class ReadArray : public virtual _impl::BasicReadArray<T> {};

template <typename T>
class WriteArray : public virtual _impl::BasicWriteArray<T> {};

template <typename T>
class ReadArrayExact : public virtual ReadArray<T>, public virtual _impl::BasicReadArrayExact<T> {};

template <typename T>
class WriteArrayExact : public virtual WriteArray<T>, public virtual _impl::BasicWriteArrayExact<T> {};

template <typename T>
class WriteArrayFrom : public virtual _impl::BasicWriteArrayFrom<T> {};

template <typename T>
class ReadArrayInto : public virtual _impl::BasicReadArrayInto<T> {};


template <>
class ReadArray<uint8_t> :
    public virtual _impl::BasicReadArray<uint8_t>,
    public virtual io::StreamRead //
{
public:
    Result<size_t, io::Error> stream_read(std::span<uint8_t> data) override;
};

template <>
class WriteArray<uint8_t> :
    public virtual _impl::BasicWriteArray<uint8_t>,
    public virtual io::StreamWrite //
{
public:
    Result<size_t, io::Error> stream_write(std::span<const uint8_t> data) override;
};

template <>
class ReadArrayExact<uint8_t> :
    public virtual ReadArray<uint8_t>,
    public virtual _impl::BasicReadArrayExact<uint8_t>,
    public virtual io::StreamReadExact //
{
public:
    Result<std::monostate, io::Error> stream_read_exact(std::span<uint8_t> data) override;
};

template <>
class WriteArrayExact<uint8_t> :
    public virtual WriteArray<uint8_t>,
    public virtual _impl::BasicWriteArrayExact<uint8_t>,
    public virtual io::StreamWriteExact //
{
public:
    Result<std::monostate, io::Error> stream_write_exact(std::span<const uint8_t> data) override;
};

template <>
class WriteArrayFrom<uint8_t> :
    public virtual _impl::BasicWriteArrayFrom<uint8_t>,
    public virtual io::WriteFromStream //
{
public:
    Result<size_t, io::Error> write_from_stream(io::StreamRead &stream, std::optional<size_t> len) override;
};

template <>
class ReadArrayInto<uint8_t> :
    public virtual _impl::BasicReadArrayInto<uint8_t>,
    public virtual io::ReadIntoStream //
{
public:
    Result<size_t, io::Error> read_into_stream(io::StreamWrite &stream, std::optional<size_t> len) override;
};

} // namespace core
