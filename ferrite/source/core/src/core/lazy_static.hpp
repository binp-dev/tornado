#pragma once

#include <atomic>
#include <mutex>

#include "maybe_uninit.hpp"

namespace core {

// NOTE: Must subject to [constant initialization](https://en.cppreference.com/w/cpp/language/constant_initialization).
template <typename T, void F(MaybeUninit<T> &)>
class LazyStatic {
private:
    std::atomic_bool initialized;
    MaybeUninit<std::mutex> mutex_;
    MaybeUninit<T> value_;

private:
    std::mutex &mutex() {
        return mutex_.assume_init();
    }
    T &value() {
        return value_.assume_init();
    }

public:
    constexpr LazyStatic() = default;
    ~LazyStatic() = default;

    LazyStatic(const LazyStatic &) = delete;
    LazyStatic &operator=(const LazyStatic &) = delete;
    LazyStatic(LazyStatic &&) = delete;
    LazyStatic &operator=(LazyStatic &&) = delete;

    void try_init() {
        if (!initialized.load()) {
            mutex_.init_in_place();
            std::lock_guard<std::mutex> guard(mutex());
            if (!initialized.load()) {
                F(value_);
                initialized.store(true);
            }
        }
    }
    T &operator*() {
        try_init();
        return value();
    }
    T *operator->() {
        return &*(*this);
    }
};

} // namespace core
