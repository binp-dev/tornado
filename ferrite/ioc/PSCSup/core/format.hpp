#pragma once

#include <string>
#include <sstream>


template <unsigned N, typename ... Args>
bool check(const char (&fmt)[N], Args ... args) {
    bool open = false, close = false;
    std::stringstream ss;
    for (int i = 0; i < N; ++i) {
        char c = fmt[i];
        if (c == '{') {
            if (!open) {
                open = true;
            } else {
                open = false;
                ss << '{';
            }
        } else if (c == '}') {
            if (!close) {
                close = true;
            } else {
                close = false;
                ss << '}';
            }
        } else {
            assert(!open && !close);
            ss << c;
        }
    }
}
