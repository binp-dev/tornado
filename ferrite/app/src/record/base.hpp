#pragma once

#include <functional>
#include <string_view>
#include <memory>
#include <type_traits>

// Abstract record interface.
class Record {
public:
    virtual std::string_view name() const = 0;
    [[nodiscard]] virtual bool request_processing() = 0;
};

// Abstract record handler.
class Handler {
public:
    virtual ~Handler() = default;
    [[nodiscard]] virtual bool is_async() const = 0;
};

template <typename H>
class HandledRecord : public virtual Record {
public:
    static_assert(std::is_base_of_v<Handler, H>);
    virtual void set_handler(std::unique_ptr<H> &&handler) = 0;
};
