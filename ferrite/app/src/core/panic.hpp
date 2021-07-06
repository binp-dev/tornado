#pragma once

#include <string>

[[noreturn]] void panic(const std::string &message = "");
[[noreturn]] void unreachable();
