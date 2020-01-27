#pragma once


template <typename T>
class slice {
    private:
    T *_data;
    const size_t _size;

    public:
    slice(T *data, size_t size) :
        _data(data),
        _size(size)
    {}
    operator slice<const T>() const {
        return slice<const T>(_data, _size);
    }

    size_t size() const {
        return _size;
    }

    T *data() {
        return _data;
    }
    const T *data() const {
        return _data;
    }

    T &operator[](size_t pos) {
        return _data[pos];
    }
    const T &operator[](size_t pos) const {
        return _data[pos];
    }
};

template <typename T>
using const_slice = slice<const T>;
