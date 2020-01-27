#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstring>
#include <cassert>

#include <channel/channel.hpp>
#include <utils/slice.hpp>

class Encoder {
private:
    double _low;
    double _high;
    int _bytes;

public:
    Encoder(double low, double high, int bytes) {
        _low = low;
        _high = high;
        _bytes = bytes;
    }

    double low() const { return _low; }
    double high() const { return _high; }
    int bytes() const { return _bytes; }

    void encode(double value, uint8_t *data) const {
        uint64_t enc;
        if (value <= low()) {
            enc = 0;
        } else if (value >= high()) {
            enc = -1;
        } else {
            enc = (1<<(bytes()*8))*((value - low())/(high() - low()));
        }
        memcpy(data, (void*)&enc, bytes());
    }
};


template <typename T, class E>
class Sender {
private:
    size_t pos = 0;
    size_t packet_size = 0;
    std::vector<uint8_t> packet;

    std::shared_ptr<Channel> channel;
    E encoder;

    const double *data = nullptr;
    size_t data_size = 0;

public:
    Sender(const Sender &dev) = delete;
    Sender &operator=(const Sender &dev) = delete;

    Sender(
        std::shared_ptr<Channel> ch,
        size_t max_packet_size,
        E encoder
    ) :
        packet(max_packet_size, 0),
        channel(std::move(ch)),
        encoder(std::move(encoder))
    {}
    
    void set_data(const double *data, size_t data_size) {
        this->data = data;
        this->data_size = data_size;
        this->pos = 0;
        this->packet_size = 0;
    }
    bool is_complete() {
        return pos >= data_size && packet_size == 0;
    }
    size_t try_send(int timeout) {
        if (packet_size == 0) {
            while(pos < data_size && packet_size + encoder.bytes() - 1 < packet.size()) {
                encoder.encode(data[pos], packet.data());
                pos += 1;
                packet_size += encoder.bytes();
            }
            if (packet_size == 0) {
                return 0;
            }
        }

        try {
            channel->send(packet.data(), packet_size, timeout);
        } catch (const Channel::TimedOut &e) {
            return 0;
        }

        size_t sent = packet_size;
        packet_size = 0;
        return sent;
    }
};


class Device {
private:
    const size_t _max_points;
    const size_t _max_transfer;

    std::vector<double> waveforms[3];
    std::atomic_bool swap_ready;
    std::mutex swap_mutex;

    std::shared_ptr<Channel> channel;
    Sender<double, Encoder> sender;
    std::vector<uint8_t> buffer;

    std::atomic_bool done;
    std::thread worker;

    void serve_loop() {
        std::cout << "[ioc] Channel serve thread started" << std::endl;
        bool sending = false;

        while(!this->done.load()) {
            if (!sending) {
                try {
                    size_t size = channel->receive(buffer.data(), buffer.size(), 10);
                    if (size < buffer.size()) {
                        buffer[size] = '\0';
                        std::cout << (char *)buffer.data() << std::endl;
                    }
                } catch (const Channel::TimedOut &e) {
                    continue;
                }
                sending = true;
            }

            if (sending) {
                if (sender.is_complete()) {
                    if (swap_ready.load()) {
                        std::lock_guard<std::mutex> guard(swap_mutex);
                        std::swap(waveforms[0], waveforms[1]);
                        swap_ready.store(false);
                    }
                    sender.set_data(waveforms[0].data(), waveforms[0].size());
                }
                if (sender.try_send(10) > 0) {
                    sending = false;
                }
            }
        }
    }

public:
    Device(const Device &dev) = delete;
    Device &operator=(const Device &dev) = delete;

    Device(
        std::unique_ptr<Channel> channel,
        size_t max_points,
        size_t max_transfer
    ) :
        _max_points(max_points),
        _max_transfer(max_transfer),

        swap_ready(false),

        channel(std::move(channel)),
        sender(this->channel, max_transfer, Encoder(0, (1<<24) - 1, 3)),
        buffer(max_transfer, 0)
    {
        for (int i = 0; i < 3; ++i) {
            waveforms[i].resize(max_points, 0.0);
        }

        done.store(false);
        worker = std::thread([this]() { this->serve_loop(); });
    }
    virtual ~Device() {
        done.store(true);
        worker.join();
    }

    void set_waveform(const_slice<double> waveform) {
        assert(waveform.size() <= max_points());
        if (!swap_ready.load()) {
            memcpy(waveforms[1].data(), waveform.data(), waveform.size());
            swap_ready.store(true);
        } else {
            memcpy(waveforms[2].data(), waveform.data(), waveform.size());
            {
                std::lock_guard<std::mutex> guard(swap_mutex);
                std::swap(waveforms[1], waveforms[2]);
                swap_ready.store(true);
            }
        }
    }

    size_t max_points() const {
        return _max_points;
    }
    size_t max_transfer() const {
        return _max_transfer;
    }
};
