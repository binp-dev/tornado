#pragma once

#include <vector>
#include <optional>
#include <iterator>
#include <cstring>

#include <core/maybe_uninit.hpp>
#include <core/stream.hpp>
#include <core/io.hpp>

#include "slice.hpp"

namespace core {

template <typename T>
struct VecDeque;
template <typename T>
struct VecDequeView;

namespace _impl {

template <typename T>
class BasicVecDeque {
private:
    std::vector<MaybeUninit<T>> data_;
    size_t front_ = 0;
    size_t back_ = 0;

    [[nodiscard]] size_t mod() const {
        return data_.size();
    }

public:
    BasicVecDeque() = default;
    explicit BasicVecDeque(size_t cap) : data_(cap + 1) {}

    ~BasicVecDeque() {
        clear();
    }

    BasicVecDeque(const BasicVecDeque &other);
    BasicVecDeque &operator=(const BasicVecDeque &other);

    BasicVecDeque(BasicVecDeque &&other);
    BasicVecDeque &operator=(BasicVecDeque &&other);

public:
    [[nodiscard]] size_t capacity() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;

private:
    [[nodiscard]] T pop_back_unchecked();
    [[nodiscard]] T pop_front_unchecked();

    void push_back_unchecked(T &&value);
    void push_front_unchecked(T &&value);

    void append_unchecked(BasicVecDeque<T> &other);
    void append_copy_unchecked(const BasicVecDeque<T> &other);

    void reserve_mod(size_t new_mod);

protected:
    void grow();
    void grow_to_free(size_t count);

public:
    void clear();

    void reserve(size_t new_cap);

    void append(BasicVecDeque &other);
    void append_copy(const BasicVecDeque &other);

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
class BasicVecDequeView {
private:
    Slice<T> first_;
    Slice<T> second_;

public:
    // Empty view.
    BasicVecDequeView() = default;
    BasicVecDequeView(Slice<T> first, Slice<T> second) : first_(first), second_(second) {}

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
class StreamVecDeque : public BasicVecDeque<T> {
public:
    using BasicVecDeque<T>::BasicVecDeque;
};

template <typename T>
class StreamVecDeque<T, true> :
    public BasicVecDeque<T>,
    public virtual ReadArrayExact<T>,
    public virtual WriteArrayExact<T>,
    public virtual WriteArrayFrom<T>,
    public virtual ReadArrayInto<T> //
{
public:
    using BasicVecDeque<T>::BasicVecDeque;

    [[nodiscard]] size_t read_array(std::span<T> data) override;
    [[nodiscard]] bool read_array_exact(std::span<T> data) override;

    [[nodiscard]] size_t write_array(std::span<const T> data) override;
    [[nodiscard]] bool write_array_exact(std::span<const T> data) override;

    size_t write_array_from(ReadArray<T> &stream, std::optional<size_t> len) override;
    size_t read_array_into(WriteArray<T> &stream, std::optional<size_t> len) override;
};

template <typename T, bool = std::is_trivial_v<T>>
class StreamVecDequeView : public BasicVecDequeView<T> {
public:
    using BasicVecDequeView<T>::BasicVecDequeView;
};

template <typename T>
class StreamVecDequeView<T, true> :
    public BasicVecDequeView<T>,
    public virtual ReadArrayExact<T>,
    public virtual ReadArrayInto<T> //
{
public:
    using _impl::BasicVecDequeView<T>::BasicVecDequeView;

    [[nodiscard]] size_t read_array(std::span<T> data) override;
    [[nodiscard]] bool read_array_exact(std::span<T> data) override;

    size_t read_array_into(WriteArray<T> &stream, std::optional<size_t> len) override;
};


} // namespace _impl

template <typename T>
class VecDeque : public _impl::StreamVecDeque<T> {
public:
    using _impl::StreamVecDeque<T>::StreamVecDeque;
};

template <typename T>
class VecDequeView : public _impl::StreamVecDequeView<T> {
public:
    using _impl::StreamVecDequeView<T>::StreamVecDequeView;
};

} // namespace core

#include "vec_deque.hxx"
