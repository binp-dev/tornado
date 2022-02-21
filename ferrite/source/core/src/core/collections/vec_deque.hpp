#pragma once

#include <vector>
#include <optional>
#include <iterator>
#include <cstring>

#include <core/maybe_uninit.hpp>
#include <core/io.hpp>

#include "slice.hpp"

template <typename T>
struct VecDeque;
template <typename T>
struct VecDequeView;

template <typename T>
class _VecDequeView {
private:
    Slice<T> first_;
    Slice<T> second_;

public:
    // Empty view.
    _VecDequeView() = default;
    _VecDequeView(Slice<T> first, Slice<T> second) : first_(first), second_(second) {}

    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool is_empty() const;

    void clear();

    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_back();
    [[nodiscard]] std::optional<std::reference_wrapper<T>> pop_front();

    size_t skip_front(size_t count);
    size_t skip_back(size_t count);

    [[nodiscard]] std::pair<Slice<T>, Slice<T>> as_slices();
    [[nodiscard]] std::pair<Slice<const T>, Slice<const T>> as_slices() const;
};

template <typename T>
class _VecDeque {
private:
    std::vector<MaybeUninit<T>> data_;
    size_t front_ = 0;
    size_t back_ = 0;

    [[nodiscard]] size_t mod() const {
        return data_.size();
    }

public:
    _VecDeque() = default;
    explicit _VecDeque(size_t cap) : data_(cap + 1) {}

    ~_VecDeque() {
        clear();
    }

    _VecDeque(const _VecDeque &other);
    _VecDeque &operator=(const _VecDeque &other);

    _VecDeque(_VecDeque &&other);
    _VecDeque &operator=(_VecDeque &&other);

public:
    [[nodiscard]] size_t capacity() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool is_empty() const;

private:
    [[nodiscard]] T pop_back_unchecked();
    [[nodiscard]] T pop_front_unchecked();

    void push_back_unchecked(T &&value);
    void push_front_unchecked(T &&value);

    void append_unchecked(_VecDeque &other);
    void append_copy_unchecked(const _VecDeque &other);

    void reserve_mod(size_t new_mod);

protected:
    void grow();
    void grow_to_free(size_t count);

public:
    void clear();

    void reserve(size_t new_cap);

    void append(_VecDeque &other);
    void append_copy(const _VecDeque &other);

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
struct VecDeque final : public _VecDeque<T> {
    using _VecDeque<T>::_VecDeque;
};

template <>
struct VecDeque<uint8_t> final : public _VecDeque<uint8_t>, public io::ReadExact, public io::WriteExact {
    using _VecDeque<uint8_t>::_VecDeque;

    Result<size_t, io::Error> read(uint8_t *data, size_t len) override;
    Result<std::monostate, io::Error> read_exact(uint8_t *data, size_t len) override;

    Result<size_t, io::Error> write(const uint8_t *data, size_t len) override;
    Result<std::monostate, io::Error> write_exact(const uint8_t *data, size_t len) override;

    Result<size_t, io::Error> read_from(io::Read &read, std::optional<size_t> len);
    Result<size_t, io::Error> write_to(io::Write &write, std::optional<size_t> len);
};

template <typename T>
struct VecDequeView final : public _VecDequeView<T> {
    using _VecDequeView<T>::_VecDequeView;
};

#include "vec_deque.hxx"
