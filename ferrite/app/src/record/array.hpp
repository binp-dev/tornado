#pragma once

#include "base.hpp"

template <typename T>
class InputArrayRecord : public virtual Record {
public:
    // TODO: Use constant array view
    virtual const T *data() const = 0;

    virtual size_t max_length() const = 0;
    virtual size_t length() const = 0;

    std::pair<const T *, size_t> slice() const {
        return std::pair(data<T>(), length());
    }
};

template <typename T>
class OutputArrayRecord : public virtual InputArrayRecord<T> {
public:
    // TODO: Use extendable array view
    virtual T *data() = 0;

    virtual bool set_length(size_t length) = 0;

    std::pair<T *, size_t> slice() {
        return std::pair(data<T>(), length());
    }
};

template <typename T>
class InputArrayHandler : public Handler {
public:
    virtual void read(const InputArrayRecord<T> &record) = 0;
};

template <typename T>
class OutputArrayHandler : public Handler {
public:
    virtual void write(OutputArrayRecord<T> &record) = 0;
};
