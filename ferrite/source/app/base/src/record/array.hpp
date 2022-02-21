#pragma once

#include "base.hpp"

template <typename T>
class InputArrayHandler;
template <typename T>
class OutputArrayHandler;

// Record for reading an array of values from device.
template <typename T>
class InputArrayRecord :
    public virtual Record,
    public HandledRecord<InputArrayHandler<T>>
{
public:
    // TODO: Use extendable array view
    virtual const T *data() const = 0;
    virtual T *data() = 0;

    virtual size_t max_length() const = 0;
    virtual size_t length() const = 0;

    [[nodiscard]] virtual bool set_length(size_t length) = 0;
    [[nodiscard]] virtual bool set_data(const T *new_data, size_t length) = 0;
};

// Record for writing an array of values to device.
template <typename T>
class OutputArrayRecord :
    public virtual Record,
    public HandledRecord<OutputArrayHandler<T>>
{
public:
    // TODO: Use constant array view
    virtual const T *data() const = 0;

    virtual size_t max_length() const = 0;
    virtual size_t length() const = 0;
};

template <typename T>
class InputArrayHandler : public Handler {
public:
    virtual void read(InputArrayRecord<T> &record) = 0;
    virtual void set_read_request(InputArrayRecord<T> &record, std::function<void()> &&callback) = 0;
};

template <typename T>
class OutputArrayHandler : public Handler {
public:
    virtual void write(OutputArrayRecord<T> &record) = 0;
};