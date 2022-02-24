#pragma once

#include <vector>
#include <optional>
#include <iterator>
#include <cstring>

#include <core/maybe_uninit.hpp>
#include <core/stream.hpp>
#include <core/io.hpp>

#include "slice.hpp"

template <typename T>
struct VecDeque;
template <typename T>
struct VecDequeView;


template <typename T>
class __VecDeque {
private:
    std::vector<MaybeUninit<T>> data_;
    size_t front_ = 0;
    size_t back_ = 0;

    [[nodiscard]] size_t mod() const {
        return data_.size();
    }

public:
    __VecDeque() = default;
    explicit __VecDeque(size_t cap) : data_(cap + 1) {}

    ~__VecDeque() {
        clear();
    }

    __VecDeque(const __VecDeque &other);
    __VecDeque &operator=(const __VecDeque &other);

    __VecDeque(__VecDeque &&other);
    __VecDeque &operator=(__VecDeque &&other);

public:
    [[nodiscard]] size_t capacity() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;

private:
    [[nodiscard]] T pop_back_unchecked();
    [[nodiscard]] T pop_front_unchecked();

    void push_back_unchecked(T &&value);
    void push_front_unchecked(T &&value);

    void append_unchecked(__VecDeque<T> &other);
    void append_copy_unchecked(const __VecDeque<T> &other);

    void reserve_mod(size_t new_mod);

protected:
    void grow();
    void grow_to_free(size_t count);

public:
    void clear();

    void reserve(size_t new_cap);

    void append(__VecDeque &other);
    void append_copy(const __VecDeque &other);

    [[nodiscard]] std::optional<T> pop_back();
    [[nodiscard]] std::optional<T> pop_front();

    void push_back(T &&value);
    void push_front(T &&value);
    void push_back(const T &value);
    void push_front(const T &value);

    size_t skip_front(size_t count);
    size_t skip_back(size_t count);

    [[nodiscard]] std::pair<Slice<T>, Slice<T>> as_slices();
    [[nodiscard]] std::pair<Slice<const T>, Slice<const T>> as_slices() const;

protected:
    [[nodiscard]] std::pair<Slice<MaybeUninit<T>>, Slice<MaybeUninit<T>>> free_space_as_slices();

    // Elements at expanded positions must be initialized.
    // Count must be less or equal to free space.
    void expand_front(size_t count);
    void expand_back(size_t count);

public:
    [[nodiscard]] VecDequeView<T> view();
    [[nodiscard]] VecDequeView<const T> view() const;
};


template <typename T>
class __VecDequeView {
private:
    Slice<T> first_;
    Slice<T> second_;

public:
    // Empty view.
    __VecDequeView() = default;
    __VecDequeView(Slice<T> first, Slice<T> second) : first_(first), second_(second) {}

    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;

    void clear();

    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_back();
    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_front();

    size_t skip_front(size_t count);
    size_t skip_back(size_t count);

    [[nodiscard]] std::pair<Slice<T>, Slice<T>> as_slices();
    [[nodiscard]] std::pair<Slice<const T>, Slice<const T>> as_slices() const;
};


template <typename T, bool = std::is_trivial_v<T>>
class _VecDeque : public __VecDeque<T> {
public:
    using __VecDeque<T>::__VecDeque;
};

template <typename T>
class _VecDeque<T, true> :
    public __VecDeque<T>,
    public virtual StreamReadExact<T>,
    public virtual StreamWriteExact<T>,
    public virtual StreamReadFrom<T>,
    public virtual StreamWriteInto<T> //
{
public:
    using __VecDeque<T>::__VecDeque;

    [[nodiscard]] size_t stream_read(T *data, size_t len) override;
    [[nodiscard]] bool stream_read_exact(T *data, size_t len) override;

    [[nodiscard]] size_t stream_write(const T *data, size_t len) override;
    [[nodiscard]] bool stream_write_exact(const T *data, size_t len) override;

    [[nodiscard]] size_t stream_read_from(StreamRead<T> &stream, std::optional<size_t> len) override;
    [[nodiscard]] size_t stream_write_into(StreamWrite<T> &stream, std::optional<size_t> len) override;
};

template <typename T, bool = std::is_trivial_v<T>>
class _VecDequeView : public __VecDequeView<T> {
public:
    using __VecDequeView<T>::__VecDequeView;
};

template <typename T>
class _VecDequeView<T, true> :
    public __VecDequeView<T>,
    public virtual StreamReadExact<T>,
    public virtual StreamWriteInto<T> //
{
public:
    using __VecDequeView<T>::__VecDequeView;

    [[nodiscard]] size_t stream_read(T *data, size_t len) override;
    [[nodiscard]] bool stream_read_exact(T *data, size_t len) override;

    [[nodiscard]] size_t stream_write_into(StreamWrite<T> &stream, std::optional<size_t> len) override;
};


template <typename T>
class VecDeque final : public _VecDeque<T> {
public:
    using _VecDeque<T>::_VecDeque;
};

template <>
class VecDeque<uint8_t> final :
    public _VecDeque<uint8_t>,
    public virtual io::ReadExact,
    public virtual io::WriteExact,
    public virtual io::ReadFrom,
    public virtual io::WriteInto //
{
public:
    using _VecDeque<uint8_t>::_VecDeque;

    Result<size_t, io::Error> read(uint8_t *data, size_t len) override;
    Result<std::monostate, io::Error> read_exact(uint8_t *data, size_t len) override;

    Result<size_t, io::Error> write(const uint8_t *data, size_t len) override;
    Result<std::monostate, io::Error> write_exact(const uint8_t *data, size_t len) override;

    Result<size_t, io::Error> read_from(io::Read &stream, std::optional<size_t> len) override;
    Result<size_t, io::Error> write_into(io::Write &stream, std::optional<size_t> len) override;
};

template <typename T>
class VecDequeView final : public _VecDequeView<T> {
public:
    using _VecDequeView<T>::_VecDequeView;
};

template <>
class VecDequeView<uint8_t> final :
    public _VecDequeView<uint8_t>,
    public virtual io::ReadExact,
    public virtual io::WriteInto //
{
public:
    using _VecDequeView<uint8_t>::_VecDequeView;

    Result<size_t, io::Error> read(uint8_t *data, size_t len) override;
    Result<std::monostate, io::Error> read_exact(uint8_t *data, size_t len) override;

    Result<size_t, io::Error> write_into(io::Write &stream, std::optional<size_t> len) override;
};

#include "vec_deque.hxx"
