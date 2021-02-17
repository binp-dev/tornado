#include "base.hpp"

std::ostream &operator<<(std::ostream &o, const Channel::ErrorKind &ek) {
    switch (ek)
    {
    case Channel::ErrorKind::IoError:
        o << "IoError";
        break;
    case Channel::ErrorKind::TimedOut:
        o << "TimedOut";
        break;
    }
    return o;
}

std::ostream &operator<<(std::ostream &o, const Channel::Error &e) {
    return o << "Channel::Error(" << e.kind << ": " << e.message << ")";
}
