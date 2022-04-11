#pragma once

#include <string>

#include "format.hpp"

void set_panic_hook(void (*hook)());

namespace panic_impl {

[[noreturn]] void panic_with_location(const char *func, const char *file, size_t line, const std::string &message = "");

} // namespace panic_impl

#define core_panic(...) panic_impl::panic_with_location(__FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)

#define core_unimplemented(...) core_panic("Unimplemented" __VA_ARGS__)

#define core_unreachable() \
    __builtin_unreachable(); \
    core_panic("Unreachable code reached")
