#pragma once

#include <variant>
#include "panic.hpp"


template <typename T>
struct Ok final {
    T value;

    explicit Ok(const T &t) : value(t) {}
    explicit Ok(T &&t) : value(std::move(t)) {}
};

template <typename E>
struct Err final {
    E value;

    explicit Err(const E &e) : value(e) {}
    explicit Err(E &&e) : value(std::move(e)) {}
};


template <typename T, typename E>
struct [[nodiscard]] Result final {
    std::variant<E, T> variant;

    explicit Result(const T &t) : variant(std::in_place_index<1>, t) {}
    explicit Result(const E &e) : variant(std::in_place_index<0>, e) {}
    explicit Result(T &&t) : variant(std::in_place_index<1>, std::move(t)) {}
    explicit Result(E &&e) : variant(std::in_place_index<0>, std::move(e)) {}

    Result(const Ok<T> &t) : variant(std::in_place_index<1>, std::move(t.value)) {}
    Result(const Err<E> &e) : variant(std::in_place_index<0>, std::move(e.value)) {}
    Result(Ok<T> &&t) : variant(std::in_place_index<1>, std::move(t.value)) {}
    Result(Err<E> &&e) : variant(std::in_place_index<0>, std::move(e.value)) {}

    Result(const Result &r) = default;
    Result(Result &&r) = default;
    Result &operator=(const Result &r) = default;
    Result &operator=(Result &&r) = default;

    bool is_ok() const { return this->variant.index() == 1;}
    bool is_err() const { return this->variant.index() == 0;}

    const T &ok() const { return std::get<1>(this->variant); }
    const E &err() const { return std::get<0>(this->variant); }
    T &ok() { return std::get<1>(this->variant); }
    E &err() { return std::get<0>(this->variant); }

    T unwrap() {
        if (this->is_err()) {
            panic("Result is Err");
        }
        return std::move(this->ok());
    }
    E unwrap_err() {
        if (this->is_ok()) {
            panic("Result is Ok");
        }
        return std::move(this->err());
    }

    bool operator==(const Result &other) const { return this->variant == other.variant; }
    bool operator==(const Ok<T> &other) const { return this->is_ok() && this->ok() == other.value; }
    bool operator==(const Err<E> &other) const { return this->is_err() && this->err() == other.value; }
    bool operator!=(const Result &other) const { return this->variant != other.variant; }
    bool operator!=(const Ok<T> &other) const { return !this->is_ok() || this->ok() != other.value; }
    bool operator!=(const Err<E> &other) const { return !this->is_err() || this->err() != other.value; }
};
