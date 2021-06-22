#include "base.hpp"

Channel::Channel(const size_t max_length) : buffer_(max_length, 0) {}

Result<std::monostate, Channel::Error> Channel::send(const ipp::MsgAppAny &message, std::optional<std::chrono::milliseconds> timeout) {
    const size_t length = message.size();
    if (length > buffer_.size()) {
        return Err(Error{ErrorKind::OutOfBounds, "Message size is greater than buffer length"});
    }
    message.store(buffer_.data());
    return this->send_raw(buffer_.data(), length, timeout);
}
Result<ipp::MsgMcuAny, Channel::Error> Channel::receive(std::optional<std::chrono::milliseconds> timeout) {
    const auto recv_res = this->receive_raw(buffer_.data(), buffer_.size(), timeout);
    if (recv_res.is_err()) {
        return Err(recv_res.err());
    }
    auto msg_res = ipp::MsgMcuAny::load(buffer_.data(), recv_res.ok());
    if (msg_res.index() != 0) {
        switch (std::get<1>(msg_res)) {
        case IPP_LOAD_OK: unreachable();
        case IPP_LOAD_OUT_OF_BOUNDS: return Err(Error{ErrorKind::OutOfBounds, "Message size is greater than received data length"});
        case IPP_LOAD_PARSE_ERROR: return Err(Error{ErrorKind::ParseError, "Received message parse error"});
        }
    }
    return Ok(std::move(std::get<0>(msg_res)));
}

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
