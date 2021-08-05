#pragma once

#include <functional>
#include <string_view>
#include <memory>
#include <type_traits>

// Abstract record interface.
class Record {
public:
    virtual std::string_view name() const = 0;
};

// Abstract record handler.
class Handler {
public:
    virtual ~Handler() = default;
    // Specifies if the record must be processed asynchronously.
    [[nodiscard]] virtual bool is_async() const = 0;
};

// Helper class for typed handler setting.
template <typename H>
class HandledRecord : public virtual Record {
public:
    static_assert(std::is_base_of_v<Handler, H>);
    // Sets a handler for the record.
    virtual void set_handler(std::unique_ptr<H> &&handler) = 0;
};
