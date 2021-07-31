#pragma once

#include "base.hpp"

template <typename T>
class InputValueHandler;
template <typename T>
class OutputValueHandler;

template <typename T>
class InputValueRecord :
    public virtual Record,
    public HandledRecord<InputValueHandler<T>>
{
public:
    virtual T value() const = 0;
};

template <typename T>
class OutputValueRecord :
    public virtual Record,
    public HandledRecord<OutputValueHandler<T>>
{
public:
    virtual T value() const = 0;
    virtual void set_value(T value) = 0;
};

template <typename T>
class InputValueHandler : public Handler {
public:
    virtual void read(InputValueRecord<T> &record) = 0;
};

template <typename T>
class OutputValueHandler : public Handler {
public:
    virtual void write(OutputValueRecord<T> &record) = 0;
    virtual void set_write_request(OutputValueRecord<T> &record, std::function<void()> &&callback) = 0;
};
