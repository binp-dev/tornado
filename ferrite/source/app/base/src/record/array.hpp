#pragma once

#include <span>

#include "base.hpp"

template <typename T>
class InputArrayHandler;
template <typename T>
class OutputArrayHandler;

// Record for reading an array of values from device.
template <typename T>
class InputArrayRecord : public virtual HandledRecord<InputArrayHandler<T>> {
public:
    virtual std::span<const T> data() const = 0;
    virtual std::span<T> data() = 0;

    virtual size_t max_length() const = 0;

    [[nodiscard]] virtual bool set_data(std::span<const T> new_data) = 0;
};

// Record for writing an array of values to device.
template <typename T>
class OutputArrayRecord : public virtual HandledRecord<OutputArrayHandler<T>> {
public:
    virtual std::span<const T> data() const = 0;
    virtual size_t max_length() const = 0;
};

template <typename T>
class InputArrayHandler : public virtual Handler {
public:
    virtual void read(InputArrayRecord<T> &record) = 0;
    virtual void set_read_request(InputArrayRecord<T> &record, std::function<void()> &&callback) = 0;
};

template <typename T>
class OutputArrayHandler : public virtual Handler {
public:
    virtual void write(OutputArrayRecord<T> &record) = 0;
};