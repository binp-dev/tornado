#pragma once

#include <atomic>
#include <mutex>
#include <memory>

template <typename T>
class LazyStatic {
private:
    std::atomic_bool initialized;
    std::mutex mutex;
    std::unique_ptr<T> value;

protected:
    virtual std::unique_ptr<T> init_value() = 0;

public:
    LazyStatic() : initialized(false) {}
    ~LazyStatic() = default;

    void try_init() {
        if (!initialized.load()) {
            std::lock_guard<std::mutex> guard(mutex);
            if (!initialized.load()) {
                value = std::move(init_value());
                initialized.store(true);
            }
        }
    }
    const T &operator*() {
        try_init();
        return value;
    }
};
