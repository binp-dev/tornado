#pragma once


#include <vector>


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
    slice(std::vector<T> &vector) :
        _data(vector.data()),
        _size(vector.size())
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

    slice<T> truncate(size_t begin, size_t end) {
        return slice<T>(data() + begin, end - begin);
    }
    slice<const T> truncate(size_t begin, size_t end) const {
        return slice<const T>(data() + begin, end - begin);
    }
};


template <typename T>
class slice<const T> {
    private:
    const T *_data;
    const size_t _size;

    public:
    slice(const T *data, size_t size) :
        _data(data),
        _size(size)
    {}
    slice(const std::vector<T> &vector) :
        _data(vector.data()),
        _size(vector.size())
    {}

    size_t size() const {
        return _size;
    }

    const T *data() const {
        return _data;
    }

    const T &operator[](size_t pos) const {
        return _data[pos];
    }

    slice<const T> truncate(size_t begin, size_t end) const {
        return slice<const T>(data() + begin, end - begin);
    }
};


template <typename T>
using const_slice = slice<const T>;
