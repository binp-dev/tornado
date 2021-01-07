#pragma once

#include <atomic>
#include <mutex>
#include <memory>

template <typename T>
class LazyStatic {
private:
    mutable std::atomic_bool initialized;
    mutable std::mutex mutex;
    mutable std::unique_ptr<T> value;

protected:
    virtual std::unique_ptr<T> init_value() const = 0;

public:
    LazyStatic() : initialized(false) {}
    ~LazyStatic() = default;

    void try_init() const {
        if (!initialized.load()) {
            std::lock_guard<std::mutex> guard(mutex);
            if (!initialized.load()) {
                value = std::move(init_value());
                initialized.store(true);
            }
        }
    }
    const T &operator*() const {
        try_init();
        return *value;
    }
    const T *operator->() const {
        return &(**this);
    }
};
