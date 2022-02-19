#pragma once

#include <vector>
#include <optional>
#include <cstring>

#include <core/maybe_uninit.hpp>
#include <core/slice.hpp>
#include <core/io.hpp>

template <typename T>
class VecDeque final {
private:
    std::vector<MaybeUninit<T>> data_;
    size_t front_ = 0;
    size_t back_ = 0;

    [[nodiscard]] size_t mod() const {
        return data_.size();
    }

public:
    VecDeque() = default;
    explicit VecDeque(size_t cap) :
        data_(cap + 1) //
    {}

    ~VecDeque() {
        clear();
    }

    VecDeque(const VecDeque &other);
    VecDeque &operator=(const VecDeque &other);

    VecDeque(VecDeque &&other);
    VecDeque &operator=(VecDeque &&other);

public:
    [[nodiscard]] size_t capacity() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool is_empty() const;

private:
    [[nodiscard]] T pop_back_unchecked();
    [[nodiscard]] T pop_front_unchecked();

    void push_back_unchecked(T &&value);
    void push_front_unchecked(T &&value);

    void append_unchecked(VecDeque &other);
    void append_copy_unchecked(const VecDeque &other);

    void reserve_mod(size_t new_mod);
    void grow();
    void grow_to_free(size_t count);

public:
    void clear();

    void reserve(size_t new_cap);

    void append(VecDeque &other);
    void append_copy(const VecDeque &other);

    [[nodiscard]] std::optional<T> pop_back();
    [[nodiscard]] std::optional<T> pop_front();

    void push_back(T &&value);
    void push_front(T &&value);
    void push_back(const T &value);
    void push_front(const T &value);

    [[nodiscard]] size_t skip_front(size_t count);
    [[nodiscard]] size_t skip_back(size_t count);

    std::pair<Slice<T>, Slice<T>> as_slices();
    std::pair<Slice<const T>, Slice<const T>> as_slices() const;

private:
    std::pair<Slice<MaybeUninit<T>>, Slice<MaybeUninit<T>>> free_space_as_slices();

    // Elements at expanded positions must be initialized.
    // Count must be less or equal to free space.
    void expand_front(size_t count);
    void expand_back(size_t count);

public:
    friend class VecDequeStream;
};


struct VecDequeStream final : public io::ReadExact, public io::WriteExact {
    VecDeque<uint8_t> queue;

    Result<size_t, io::Error> read(uint8_t *data, size_t len) override;
    Result<std::monostate, io::Error> read_exact(uint8_t *data, size_t len) override;

    Result<size_t, io::Error> write(const uint8_t *data, size_t len) override;
    Result<std::monostate, io::Error> write_exact(const uint8_t *data, size_t len) override;

    Result<size_t, io::Error> read_from(io::Read &read, std::optional<size_t> len);
    Result<size_t, io::Error> write_to(io::Write &write, std::optional<size_t> len);
};

#include "vec_deque.hxx"
