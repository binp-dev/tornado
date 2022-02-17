#include <core/io.hpp>

enum class StreamError {
    BAD_DATA,
    END_OF_STREAM,
    IO,
};

class Msg final {
public:
    [[nodiscard]] size_t packed_size() const {
        return 0;
    }

    [[nodiscard]] static Result<Msg, StreamError> load(Read &read) {
        return Ok(Msg{});
    }

    Result<std::monostate, StreamError> store(Write &write) const {}
};

/*
TEST(Io, msg_load_store) {
    RawMsg raw_msg;
    Msg msg = Msg::load(&raw_msg);
    msg.store(&raw_msg);
}
*/
