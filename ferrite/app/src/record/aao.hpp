#pragma once

#include "base.hpp"

// Abstract record interface.
template <typename T>
class AaoRecord : public virtual Record {
public:
    virtual size_t length() const = 0;
    virtual size_t max_length() const = 0;
};

// Abstract record handler.
class Handler {
private:
    const bool async_;

public:
    Handler(bool async) : async_(async) {}
    virtual ~Handler() = default;

    inline bool is_async() const {
        return async_;
    }
};
