#pragma once

#include <array>

#include "format.hpp"

namespace core {

enum class LogLevel {
    Fatal = 0,
    Error,
    Warning,
    Info,
    Debug,
    Trace,
};

namespace _impl {

constexpr std::array<const char *, size_t(LogLevel::Trace) + 1> LEVEL_NAMES = {
    "fatal",
    "error",
    "warning",
    "info",
    "debug",
    "trace",
};

template <LogLevel LEVEL, _impl::Literal FMT_STR, typename... Ts>
void log(Ts &&...args) {
    constexpr auto LOG_FMT_STR = _impl::Literal("[app:{}] ").append(FMT_STR);
    _impl::print<LOG_FMT_STR>(std::cout, true, LEVEL_NAMES[size_t(LEVEL)], std::forward<Ts>(args)...);
}

} // namespace _impl
} // namespace core

#define core_log(level, fmt, ...) ::core::_impl::log<level, fmt>(__VA_ARGS__)

/* clang-format off */
#define core_log_fatal(  fmt, ...) core_log(::core::LogLevel::Fatal,   fmt, ##__VA_ARGS__)
#define core_log_error(  fmt, ...) core_log(::core::LogLevel::Error,   fmt, ##__VA_ARGS__)
#define core_log_warning(fmt, ...) core_log(::core::LogLevel::Warning, fmt, ##__VA_ARGS__)
#define core_log_info(   fmt, ...) core_log(::core::LogLevel::Info,    fmt, ##__VA_ARGS__)
#define core_log_debug(  fmt, ...) core_log(::core::LogLevel::Debug,   fmt, ##__VA_ARGS__)
#define core_log_trace(  fmt, ...) core_log(::core::LogLevel::Trace,   fmt, ##__VA_ARGS__)
/* clang-format on */
