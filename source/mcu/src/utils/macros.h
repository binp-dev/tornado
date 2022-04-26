#pragma once

#define _concat(a, b) a##b

#define concat(...) _concat(__VA_ARGS__)
