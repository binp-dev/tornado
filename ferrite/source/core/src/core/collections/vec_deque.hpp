#pragma once

#include <vector>
#include <optional>

#include <core/maybe_uninit.hpp>


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
    explicit VecDeque(size_t cap) : data_(cap + 1) {}

    ~VecDeque() {
        clear();
    }

    VecDeque(const VecDeque &other) : VecDeque(other.size()) {
        append_copy_unchecked(other);
    }
    VecDeque &operator=(const VecDeque &other) {
        clear();
        append_copy(other);
    }
    VecDeque(VecDeque &&other) : data_(std::move(other.data_)), front_(other.front_), back_(other.back_) {
        other.front_ = 0;
        other.back_ = 0;
    }
    VecDeque &operator=(VecDeque &&other) {
        clear();

        data_ = std::move(other.data_);
        front_ = other.front_;
        back_ = other.back_;

        other.front_ = 0;
        other.back_ = 0;
    }

public:
    [[nodiscard]] size_t capacity() const {
        if (mod() > 1) {
            return mod() - 1;
        } else {
            return 0;
        }
    }

    [[nodiscard]] size_t size() const {
        if (mod() == 0) {
            return 0;
        } else {
            return ((back_ + mod()) - front_) % mod();
        }
    }

    [[nodiscard]] bool empty() const {
        return size() > 0;
    }

private:
    [[nodiscard]] T pop_back_unchecked() {
        size_t new_back = (back_ + mod() - 1) % mod();
        T &ref = data_[new_back].assume_init();
        T val(std::move(ref));
        back_ = new_back;
        ref.~T();
        return std::move(val);
    }
    [[nodiscard]] T pop_front_unchecked() {
        size_t new_front = (front_ + 1) % mod();
        T &ref = data_[front_].assume_init();
        T val(std::move(ref));
        front_ = new_front;
        ref.~T();
        return std::move(val);
    }

    void push_back_unchecked(T &&value) {
        size_t new_back = (back_ + 1) % mod();
        data_[back_].init_in_place(std::move(value));
        back_ = new_back;
    }
    void push_front_unchecked(T &&value) {
        size_t new_front = (front_ + mod() - 1) % mod();
        data_[new_front].init_in_place(std::move(value));
        front_ = new_front;
    }

    void append_unchecked(VecDeque &other) {
        while (other.front_ != other.back_) {
            data_[back_].init_in_place(std::move(other.data_[other.front_].assume_init()));
            other.front_ = (other.front_ + 1) % other.mod();
            back_ = (back_ + 1) % mod();
        }
    }

    void append_copy_unchecked(const VecDeque &other) {
        size_t front_view = other.front_;
        while (front_view != other.back_) {
            data_[back_].init_in_place(other.data_[front_view].assume_init());
            front_view = (front_view + 1) % other.mod();
            back_ = (back_ + 1) % mod();
        }
    }

    void reserve_mod(size_t new_mod) {
        if (new_mod > std::max(1, mod)) {
            VecDeque<T> new_self(new_mod - 1);
            new_self.append_unchecked(*this);
            *this = std::move(new_self);
        }
    }

    void increase() {
        size_t mod = data_.size();
        if (mod > 1) {
            reserve_mod(2 * mod);
        } else {
            reserve_mod(2);
        }
    }

public:
    void clear() {
        // Destructors aren't called automatically because of MaybeUninit.
        // Call them manually for initialized elements.
        while (front_ != back_) {
            data_[front_].assume_init().~T();
            front_ = (front_ + 1) % mod();
        }
        front_ = 0;
        back_ = 0;
    }

    void reserve(size_t new_cap) {
        reserve_mod(new_cap + 1);
    }

    void append(VecDeque &other) {
        reserve(size() + other.size());
        append_unchecked(other);
    }
    void append_copy(const VecDeque &other) {
        reserve(size() + other.size());
        append_copy_unchecked(other);
    }

    [[nodiscard]] std::optional<T> pop_back() {
        if (!empty()) {
            return pop_back_unchecked();
        } else {
            return std::nullopt;
        }
    }
    [[nodiscard]] std::optional<T> pop_front() {
        if (!empty()) {
            return pop_front_unchecked();
        } else {
            return std::nullopt;
        }
    }

    void push_back(T &&value) {
        if (size() == capacity()) {
            increase();
        }
        return push_back_unchecked(std::move(value));
    }
    void push_front(T &&value) {
        if (size() == capacity()) {
            increase();
        }
        return push_front_unchecked(std::move(value));
    }
};
