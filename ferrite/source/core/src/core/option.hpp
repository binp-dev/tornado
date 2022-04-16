#pragma once

#include <optional>

#include "panic.hpp"
#include "format.hpp"

namespace core {

template <typename T>
struct Some final {
    T value;

    constexpr explicit Some(const T &t) : value(t) {}
    constexpr explicit Some(T &&t) : value(std::move(t)) {}
};

struct None final {};


template <typename T>
struct Option final {
private:
    std::optional<T> optional_;

public:
    constexpr Option(const Some<T> &t) : optional_(std::move(t.value)) {}
    constexpr Option(Some<T> &&t) : optional_(std::move(t.value)) {}
    constexpr Option(None) : optional_(std::nullopt) {}

    constexpr Option(const Option &) = default;
    constexpr Option(Option &&) = default;
    constexpr Option &operator=(const Option &) = default;
    constexpr Option &operator=(Option &&) = default;

    constexpr Option(const std::optional<T> &opt) : optional_(opt){};
    constexpr Option(std::optional<T> &&opt) : optional_(opt){};

    constexpr bool is_some() const {
        return this->optional_.has_value();
    }
    constexpr bool is_none() const {
        return !this->optional_.has_value();
    }

    constexpr const T &some() const {
        return this->optional_.value();
    }
    constexpr T &some() {
        return this->optional_.value();
    }

    T unwrap() {
        if (this->is_none()) {
            core_panic("Option is None");
        }
        return std::move(this->some());
    }
    void unwrap_none() {
        if (this->is_some()) {
            if constexpr (Printable<T>) {
                core_panic("Option is Some({})", this->some());
            } else {
                core_panic("Option is Some");
            }
        }
    }

    constexpr bool operator==(const Option &other) const {
        return this->optional_ == other.optional_;
    }
    constexpr bool operator==(const Some<T> &other) const {
        return this->is_some() && this->some() == other.value;
    }
    constexpr bool operator==(None) const {
        return this->is_none();
    }
    constexpr bool operator!=(const Option &other) const {
        return this->optional_ != other.optional_;
    }
    constexpr bool operator!=(const Some<T> &other) const {
        return !this->is_some() || this->some() != other.value;
    }
    constexpr bool operator!=(None) const {
        return !this->is_none();
    }
};

template <typename T>
struct Print<Some<T>> {
    static void print(std::ostream &os, const Some<T> &some) {
        os << "Some(";
        if constexpr (Printable<T>) {
            os << some.value;
        }
        os << ")";
    }
};

template <>
struct Print<None> {
    inline static void print(std::ostream &os, None) {
        os << "None";
    }
};

template <typename T>
struct Print<Option<T>> {
    static void print(std::ostream &os, const Option<T> &opt) {
        os << "Option::";
        if (opt.is_some()) {
            os << "Some(";
            if constexpr (Printable<T>) {
                os << opt.some();
            }
            os << ")";
        } else {
            os << "None";
        }
    }
};

} // namespace core
