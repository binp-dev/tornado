#pragma once

// TODO: Migrate to std::format when supported.

#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <sstream>

#include "print.hpp"
#include "result.hpp"
#include "match.hpp"
#include "assert.hpp"

namespace format_impl {

template <size_t N>
struct Literal {
    char data[N];

    constexpr Literal(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }
    constexpr std::string_view view() const {
        return std::string_view(data, N - 1);
    }
    constexpr size_t size() const {
        return N - 1;
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
consteval Result<std::monostate, Error> check_format_str(const std::string_view str) {
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
                    return Err(Error{ErrorKind::TooManyArgs, i});
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
                return Err(Error{ErrorKind::UnpairedBrace, i});
            }
        }
    }
    if (opened || closed) {
        return Err(Error{ErrorKind::UnpairedBrace, str.size()});
    }
    if (arg != total_args) {
        return Err(Error{ErrorKind::TooFewArgs, str.size()});
    }
    return Ok(std::monostate());
}

template <typename... Ts>
void print_unchecked(std::ostream &stream, const std::string_view str, Ts &&...args) {
    [[maybe_unused]] auto arg_to_string = [](auto arg) {
        std::stringstream ss;
        Print<std::remove_reference_t<decltype(arg)>>::print(ss, arg);
        return ss.str();
    };
    std::vector<std::string> printed_args{arg_to_string(args)...};
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
                assert_true(arg < printed_args.size());
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
            assert_true(!opened && !closed);
            stream << c;
        }
    }
    assert_true(!opened && !closed);
    assert_true(arg == printed_args.size());
}

template <Literal FmtStr, typename... Ts>
void print(std::ostream &stream, bool newline, Ts &&...args) {
    constexpr auto check_result = check_format_str<Ts...>(FmtStr.view());
    static_assert(check_result.is_ok());
    print_unchecked(stream, FmtStr.view(), std::forward<Ts>(args)...);
    if (newline) {
        stream << std::endl;
    }
}

template <Literal FmtStr, typename... Ts>
std::string format(Ts &&...args) {
    std::stringstream stream;
    print<FmtStr>(stream, false, std::forward<Ts>(args)...);
    return stream.str();
}

} // namespace format_impl

#define println(fmt, ...) ::format_impl::println<fmt>(std::cout, true, ##__VA_ARGS__)

#define format(fmt, ...) ::format_impl::format<fmt>(__VA_ARGS__)
