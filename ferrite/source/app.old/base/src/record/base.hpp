#pragma once

#include <functional>
#include <string_view>
#include <memory>
#include <concepts>

// Abstract record handler.
class Handler {
public:
    // Specifies if the record must be processed asynchronously.
    const bool is_async;

    inline explicit Handler(bool is_async) : is_async(is_async) {}
    virtual ~Handler() = default;
};

// Abstract record.
class Record {
public:
    virtual std::string_view name() const = 0;

    // Record handler pointer (can be `nullptr`).
    virtual const Handler *handler() const = 0;
    virtual Handler *handler() = 0;
};

// Helper class for typed handler setting.
template <typename H>
    requires std::derived_from<H, Handler>
class HandledRecord : public virtual Record {
private:
    // User-defined handler for the record. *Can be empty.*
    std::unique_ptr<H> handler_;

public:
    // Record handler pointer (can be `nullptr`).
    const H *handler() const override {
        return handler_.get();
    }
    H *handler() override {
        return handler_.get();
    }

    // Sets a handler for the record.
    void set_handler(std::unique_ptr<H> &&handler) {
        handler_ = std::move(handler);
    }
};
