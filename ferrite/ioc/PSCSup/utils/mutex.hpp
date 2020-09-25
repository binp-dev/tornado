#pragma once

#include <mutex>


template<class T> class Mutex;

template<class T>
class LockGuard final {
    friend class Mutex<T>;
    
    private:
    std::mutex *mutex_ptr;
    T &data;

    LockGuard(std::mutex *mutex_ptr, T &data) :
        mutex_ptr(mutex_ptr),
        data(data)
    {
        mutex_ptr->lock();
    }
    
    public:
    ~LockGuard() {
        mutex_ptr->unlock();
    }

    LockGuard(const LockGuard &) = delete;
    LockGuard &operator=(const LockGuard &) = delete;
    
    LockGuard(LockGuard &&) = default;
    LockGuard &operator=(LockGuard &&) = default;
    
    T &operator*() {
        return data;
    }
    T *operator->() {
        return &data;
    }
    const T &operator*() const {
        return data;
    }
    const T *operator->() const {
        return &data;
    }
};

template<class T>
class LockGuardConst final {
    friend class Mutex<T>;
    
    private:
    std::mutex *mutex_ptr;
    const T &data;

    LockGuardConst(std::mutex *mutex_ptr, const T &data) :
        mutex_ptr(mutex_ptr),
        data(data)
    {
        mutex_ptr->lock();
    }
    
    public:
    ~LockGuardConst() {
        mutex_ptr->unlock();
    }

    LockGuardConst(const LockGuardConst &) = delete;
    LockGuardConst &operator=(const LockGuardConst &) = delete;
    
    LockGuardConst(LockGuardConst &&) = default;
    LockGuardConst &operator=(LockGuardConst &&) = default;

    const T &operator*() const {
        return data;
    }
    const T *operator->() const {
        return &data;
    }
};

template<class T>
class Mutex {
    
    private:
    T object;
    mutable std::mutex mutex;
    
    public:
    template <typename ... Args>
    Mutex(Args ... args) :
        object(std::forward<Args>(args) ...)
    {}
    ~Mutex() = default;

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    LockGuard<T> lock() {
        return std::move(LockGuard<T>(&mutex, object));
    }
    LockGuardConst<T> lock() const {
        return std::move(LockGuardConst<T>(&mutex, object));
    }
};
