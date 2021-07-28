#pragma once

#include <string>

[[noreturn]] void panic(const std::string &message = "");

#define unimplemented() panic("Unimplemented")

#define unreachable() \
    __builtin_unreachable(); \
    panic("Unreachable code reached")
