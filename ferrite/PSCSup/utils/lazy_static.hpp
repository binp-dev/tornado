#pragma once

#include <atomic>
#include <mutex>
#include <memory>

template <typename T>
class LazyStatic {
    private:
    std::atomic_bool initialized;
    std::mutex initialization_mutex;
    std::shared_ptr<T> value;

    protected:
    virtual std::unique_ptr<T> init() noexcept = 0;

    public:
    LazyStatic() : initialized(false) {}
    ~LazyStatic() = default;

    void try_init() {
        if (!initialized.load()) {
            std::lock_guard<std::mutex> guard(initialization_mutex);
            if (!initialized.load()) {
                value = std::shared_ptr<T>(std::move(init()));
            }
        }
    }
    std::shared_ptr<T> get() {
        try_init();
        return value;
    }
};
