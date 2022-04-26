#pragma once

// TODO: Migrate to std::format when supported.

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <optional>
#include <cassert>

#include "print.hpp"

namespace core::_impl {

template <size_t N>
struct Literal {
    static_assert(N > 0);
    char data[N];

    constexpr Literal() = default;
    constexpr Literal(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }

    constexpr std::string_view view() const {
        return std::string_view(data, N - 1);
    }
    constexpr size_t size() const {
        return N - 1;
    }

    template <size_t M>
    constexpr Literal<N + M - 1> append(Literal<M> other) const {
        Literal<N + M - 1> result;
        std::copy_n(this->data, N - 1, result.data);
        std::copy_n(other.data, M - 1, result.data + N - 1);
        return result;
    }
};

enum class ErrorKind {
    TooManyArgs,
    TooFewArgs,
    UnpairedBrace,
};

struct Error {
    ErrorKind kind;
    size_t pos;
};

template <typename... Ts>
consteval std::optional<Error> check_format_str(const std::string_view str) {
    constexpr size_t total_args = sizeof...(Ts);
    size_t arg = 0;
    bool opened = false;
    bool closed = false;
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (c == '{') {
            if (opened) {
                opened = false;
            } else {
                opened = true;
            }
        } else if (c == '}') {
            if (opened) {
                if (arg >= total_args) {
                    return Error{ErrorKind::TooManyArgs, i};
                }
                arg += 1;
                opened = false;
            } else if (closed) {
                closed = false;
            } else {
                closed = true;
            }
        } else {
            if (opened || closed) {
                return Error{ErrorKind::UnpairedBrace, i};
            }
        }
    }
    if (opened || closed) {
        return Error{ErrorKind::UnpairedBrace, str.size()};
    }
    if (arg != total_args) {
        return Error{ErrorKind::TooFewArgs, str.size()};
    }
    return std::nullopt;
}

template <typename... Ts>
void print_unchecked(std::ostream &stream, const std::string_view str, Ts &&...args) {
    [[maybe_unused]] auto arg_to_string = [](auto &&arg) {
        std::stringstream ss;
        Print<std::remove_cvref_t<decltype(arg)>>::print(ss, arg);
        return ss.str();
    };
    std::vector<std::string> printed_args{arg_to_string(std::forward<Ts>(args))...};
    size_t arg = 0;
    bool opened = false;
    bool closed = false;
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (c == '{') {
            if (opened) {
                stream << '{';
                opened = false;
            } else {
                opened = true;
            }
        } else if (c == '}') {
            if (opened) {
                assert(arg < printed_args.size());
                stream << printed_args[arg];
                arg += 1;
                opened = false;
            } else if (closed) {
                stream << '}';
                closed = false;
            } else {
                closed = true;
            }
        } else {
            assert(!opened && !closed);
            stream << c;
        }
    }
    assert(!opened && !closed);
    assert(arg == printed_args.size());
}

template <Literal FMT_STR, typename... Ts>
void print(std::ostream &stream, bool newline, Ts &&...args) {
    constexpr auto check_error = check_format_str<Ts...>(FMT_STR.view());
    static_assert(!check_error.has_value(), "Format error");
    print_unchecked(stream, FMT_STR.view(), std::forward<Ts>(args)...);
    if (newline) {
        stream << std::endl;
    }
}

template <Literal FMT_STR, typename... Ts>
std::string format(Ts &&...args) {
    std::stringstream stream;
    print<FMT_STR>(stream, false, std::forward<Ts>(args)...);
    return stream.str();
}

} // namespace core::_impl

#define core_write(stream, fmt_str, ...) ::core::_impl::print<fmt_str>((stream), false, ##__VA_ARGS__)
#define core_writeln(stream, fmt_str, ...) ::core::_impl::print<"" fmt_str>((stream), true, ##__VA_ARGS__)

#define core_print(...) core_write(std::cout, ##__VA_ARGS__)
#define core_println(...) core_writeln(std::cout, ##__VA_ARGS__)

#define core_format(fmt_str, ...) ::core::_impl::format<fmt_str>(__VA_ARGS__)
