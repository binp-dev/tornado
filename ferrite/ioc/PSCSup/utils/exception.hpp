#pragma once

#include <exception>
#include <string>
#include <sstream>


// FIXME: Use libunwind to print stack trace
class Exception : public std::exception {
private:
    std::string message;
    mutable std::string text;
    mutable bool text_ready = false;

public:
    Exception() = default;
    Exception(const char *msg) : message(msg) {}
    Exception(const std::string &msg) : message(msg) {}
    Exception(std::string &&msg) : message(msg) {}

    virtual const char *what() const noexcept override {
        std::stringstream stream;
        if (!text_ready) {
            stream << message << std::endl;
            text = std::move(stream.str());
            text_ready = true;
        }
        return text.c_str();
    }
};
