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

struct Subst {
    size_t index;
};

struct Error {
    size_t pos;
    std::string msg;
};

using Token = std::variant<std::string, Subst>;

template <size_t N>
struct Literal {
    char data[N];

    constexpr Literal(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }
    constexpr std::string_view view() const {
        return std::string_view(data, N);
    }
};

template <typename... Ts>
consteval Result<std::vector<Token>, Error> parse_format_str(const std::string_view str) {
    constexpr size_t total_args = sizeof...(Ts);
    size_t arg = 0;
    std::vector<Token> tokens;
    std::string buffer;
    bool opened = false;
    bool closed = false;
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (c == '{') {
            if (opened) {
                buffer.push_back('{');
                opened = false;
            } else {
                opened = true;
            }
        } else if (c == '}') {
            if (opened) {
                tokens.push_back(std::move(buffer));
                if (arg >= total_args) {
                    return Err(Error{i, "Too many args"});
                }
                tokens.push_back(Subst(arg));
                arg += 1;
                opened = false;
            } else if (closed) {
                buffer.push_back('}');
                closed = false;
            } else {
                closed = true;
            }
        } else {
            if (opened || closed) {
                return Err(Error{i, "Unpaired brace"});
            }
            buffer.push_back(c);
        }
    }
    if (opened || closed) {
        return Err(Error{str.length(), "Unpaired brace"});
    }
    if (arg != total_args) {
        return Err(Error{str.length(), "Too few args"});
    }
    return Ok(std::move(tokens));
}

template <Literal FMT_STR, typename... Ts>
void format_impl(std::ostream &os, Ts &&...args) {
    constexpr auto tokens = parse_format_str<Ts...>(FMT_STR.view()).unwrap();
    auto arg_to_string = [](auto arg) {
        std::stringstream ss;
        Print<std::remove_reference_t<decltype(arg)>>::print(ss, arg);
        return ss.str();
    };
    std::vector<std::string> printed_args{arg_to_string(args)...};
    for (Token token : tokens) {
        overloaded([&](const std::string &str) { os << str; }, [&](Subst subst) { os << printed_args[subst.index]; });
    }
}
