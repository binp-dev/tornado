#pragma once

#include <string>
#include <memory>

#include <czmq.h>

#include "channel.hpp"

class ZmqChannel : public Channel {
private:
    class ZSock;

    class ZFrame final {
    private:
        zframe_t *raw;
    public:
        ZFrame(zframe_t *raw) : raw(raw) {}
        ZFrame(const uint8_t *data, size_t length) :
            ZFrame(zframe_new(data, length))
        {}
        ~ZFrame() {
            if (raw != nullptr) {
                zframe_destroy(&raw);
            }
        }
        ZFrame(const ZFrame&) = delete;
        ZFrame &operator=(const ZFrame&) = delete;
        ZFrame(ZFrame&&) = default;
        ZFrame &operator=(ZFrame&&) = default;

        uint8_t *data() {
            return zframe_data(raw);
        }
        const uint8_t *data() const {
            return zframe_data(raw);
        }
        size_t size() const {
            return zframe_size(raw);
        }

        friend class ZSock;
    };

    class ZSock final {
    private:
        zsock_t *sock;
        zpoller_t *poller;
    public:
        ZSock(const std::string &host) :
            sock(zsock_new(ZMQ_REQ)),
            poller(zpoller_new(sock))
        {
            if (zsock_connect(sock, host.c_str()) != 0) {
                throw Channel::IoError();
            }
        }
        ~ZSock() {
            zpoller_destroy(&poller);
            zsock_destroy(&sock);
        }
        ZSock(const ZSock&) = delete;
        ZSock &operator=(const ZSock&) = delete;
        ZSock(ZSock&&) = default;
        ZSock &operator=(ZSock&&) = default;

        const zsock_t *raw() const {
            return sock;
        }
        zsock_t *raw() {
            return sock;
        }

        void send(ZFrame &&frame) {
            if (zframe_send(&frame.raw, sock, ZMQ_NOBLOCK) != 0) {
                throw Channel::IoError();
            } else {
                frame.raw = nullptr;
            }
        }
        ZFrame receive(msec timeout) {
            if (zpoller_wait(poller, timeout.count()) == nullptr) {
                throw Channel::TimedOut();
            }
            zframe_t *raw_frame = zframe_recv(sock);
            if (raw_frame != nullptr) {
                throw Channel::IoError();
            }
            return std::move(ZFrame(raw_frame));
        }
    };

private:
    std::string host;
    ZSock sock;

public:
    ZmqChannel(const std::string &host) :
        host(host),
        sock(host)
    {}
    ~ZmqChannel() override = default;

    void send(const uint8_t *bytes, size_t length, msec timeout) override {
        sock.send(ZFrame(bytes, length));
    }
    size_t receive(uint8_t *bytes, size_t max_length, msec timeout) override {
        ZFrame frame = std::move(sock.receive(timeout));
        size_t length = std::min(frame.size(), max_length);
        memcpy(bytes, frame.data(), length);
        return frame.size();
    }
};
