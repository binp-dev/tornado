#pragma once

#include <string>

void set_panic_hook(void(*hook)());

[[noreturn]] void __panic_with_location(const char *func, const char *file, size_t line, const std::string &message = "");

#define panic(...) __panic_with_location(__FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)

#define unimplemented(...) panic("Unimplemented" __VA_ARGS__)

#define unreachable() \
    __builtin_unreachable(); \
    panic("Unreachable code reached")
