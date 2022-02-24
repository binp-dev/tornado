#include "io.hpp"

namespace io {

[[nodiscard]] size_t StreamReadWrapper::stream_read(uint8_t *data, size_t len) {
    auto res = read.read(data, len);
    if (res.is_ok()) {
        return res.ok();
    } else {
        error = res.unwrap_err();
        return 0;
    }
}

[[nodiscard]] size_t StreamWriteWrapper::stream_write(const uint8_t *data, size_t len) {
    auto res = write.write(data, len);
    if (res.is_ok()) {
        return res.ok();
    } else {
        error = res.unwrap_err();
        return 0;
    }
}

} // namespace io

std::ostream &operator<<(std::ostream &o, const io::ErrorKind &ek) {
    switch (ek) {
    case io::ErrorKind::NotFound:
        o << "Not Found";
        break;
    case io::ErrorKind::UnexpectedEof:
        o << "Unexpected Eof";
        break;
    case io::ErrorKind::InvalidData:
        o << "Invalid Data";
        break;
    case io::ErrorKind::TimedOut:
        o << "Timed Out";
        break;
    case io::ErrorKind::Other:
        o << "Other";
        break;
    }
    return o;
}

std::ostream &operator<<(std::ostream &o, const io::Error &e) {
    return o << "io::Error(" << e.kind << ": " << e.message << ")";
}
