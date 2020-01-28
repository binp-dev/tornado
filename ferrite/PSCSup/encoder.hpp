#pragma once

#include <cstdint>
#include <cstring>

#include <utils/slice.hpp>


template <typename T>
class ArrayEncoder {
public:
    virtual ~ArrayEncoder() = default;
    virtual std::pair<size_t, size_t> encode(
        uint8_t *dst, size_t dst_len,
        const T *src, size_t src_len
    ) const;
};

template <typename T>
class SizedArrayEncoder : public ArrayEncoder<T> {
private:
    size_t _size;
public:
    SizedArrayEncoder(size_t size) : _size(size) {}
    size_t size() const {
        return _size;
    }
};

class LinearEncoder : public SizedArrayEncoder<double> {
private:
    double _low;
    double _high;

    uint64_t encode_value(double value) const {
        uint64_t enc;
        if (value <= _low) {
            enc = 0;
        } else if (value >= _high) {
            enc = -1;
        } else {
            enc = (1<<(size()*8))*((value - _low)/(_high - _low));
        }
        return enc;
    }

public:
    LinearEncoder(double low, double high, size_t bytes) :
        SizedArrayEncoder(bytes)
    {
        _low = low;
        _high = high;
    }

    double low() const { return _low; }
    double high() const { return _high; }

    std::pair<size_t, size_t> encode(
        uint8_t *dst, size_t dst_len,
        const double *src, size_t src_len
    ) const override {
        const size_t bytes = size();
        size_t len = std::min(src_len, dst_len/bytes);
        for (size_t i = 0; i < len; ++i) {
            uint64_t enc = encode_value(src[i]);
            memcpy(dst + i*bytes, (void*)&enc, bytes);
        }
        
        return std::make_pair(len*bytes, len);
    }
};
