#include "io.hpp"

namespace io {

Result<size_t, Error> ReadExact::read(uint8_t *data, size_t len) {
    auto res = read_exact(data, len);
    if (res.is_err()) {
        return Err(res.unwrap_err());
    }
    return Ok(len);
}

Result<size_t, Error> WriteExact::write(const uint8_t *data, size_t len) {
    auto res = write_exact(data, len);
    if (res.is_err()) {
        return Err(res.unwrap_err());
    }
    return Ok(len);
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
