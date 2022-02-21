#include "vec_deque.hpp"


template <typename T>
_VecDeque<T>::_VecDeque(const _VecDeque &other) : _VecDeque(other.size()) {
    append_copy_unchecked(other);
}

template <typename T>
_VecDeque<T> &_VecDeque<T>::operator=(const _VecDeque<T> &other) {
    clear();
    append_copy(other);
    return *this;
}

template <typename T>
_VecDeque<T>::_VecDeque(_VecDeque &&other) : data_(std::move(other.data_)), front_(other.front_), back_(other.back_) {
    other.front_ = 0;
    other.back_ = 0;
}

template <typename T>
_VecDeque<T> &_VecDeque<T>::operator=(_VecDeque<T> &&other) {
    clear();

    data_ = std::move(other.data_);
    front_ = other.front_;
    back_ = other.back_;

    other.front_ = 0;
    other.back_ = 0;

    return *this;
}

template <typename T>
size_t _VecDeque<T>::capacity() const {
    if (mod() > 1) {
        return mod() - 1;
    } else {
        return 0;
    }
}

template <typename T>
size_t _VecDeque<T>::size() const {
    if (mod() == 0) {
        return 0;
    } else {
        return ((back_ + mod()) - front_) % mod();
    }
}

template <typename T>
bool _VecDeque<T>::is_empty() const {
    return size() == 0;
}

template <typename T>
T _VecDeque<T>::pop_back_unchecked() {
    size_t new_back = (back_ + mod() - 1) % mod();
    T &ref = data_[new_back].assume_init();
    T val(std::move(ref));
    back_ = new_back;
    ref.~T();
    return val;
}

template <typename T>
T _VecDeque<T>::pop_front_unchecked() {
    size_t new_front = (front_ + 1) % mod();
    T &ref = data_[front_].assume_init();
    T val(std::move(ref));
    front_ = new_front;
    ref.~T();
    return val;
}

template <typename T>
void _VecDeque<T>::push_back_unchecked(T &&value) {
    size_t new_back = (back_ + 1) % mod();
    data_[back_].init_in_place(std::move(value));
    back_ = new_back;
}

template <typename T>
void _VecDeque<T>::push_front_unchecked(T &&value) {
    size_t new_front = (front_ + mod() - 1) % mod();
    data_[new_front].init_in_place(std::move(value));
    front_ = new_front;
}

template <typename T>
void _VecDeque<T>::append_unchecked(_VecDeque<T> &other) {
    while (other.front_ != other.back_) {
        data_[back_].init_in_place(std::move(other.data_[other.front_].assume_init()));
        other.front_ = (other.front_ + 1) % other.mod();
        back_ = (back_ + 1) % mod();
    }
}

template <typename T>
void _VecDeque<T>::append_copy_unchecked(const _VecDeque<T> &other) {
    size_t front_view = other.front_;
    while (front_view != other.back_) {
        data_[back_].init_in_place(other.data_[front_view].assume_init());
        front_view = (front_view + 1) % other.mod();
        back_ = (back_ + 1) % mod();
    }
}

template <typename T>
void _VecDeque<T>::reserve_mod(size_t new_mod) {
    if (new_mod > std::max(size_t(1), mod())) {
        _VecDeque<T> new_self(new_mod - 1);
        new_self.append_unchecked(*this);
        *this = std::move(new_self);
    }
}

template <typename T>
void _VecDeque<T>::grow() {
    if (mod() > 1) {
        reserve_mod(2 * mod());
    } else {
        reserve_mod(2);
    }
}

template <typename T>
void _VecDeque<T>::grow_to_free(size_t count) {
    size_t new_mod = std::max(mod(), size_t(2));
    while (new_mod < size() + count + 1) {
        new_mod = 2 * new_mod;
    }
    reserve_mod(new_mod);
}


template <typename T>
void _VecDeque<T>::clear() {
    if (!std::is_trivial_v<T>) {
        // Destructors aren't called automatically because of MaybeUninit.
        // Call them manually for initialized elements.
        while (front_ != back_) {
            data_[front_].assume_init().~T();
            front_ = (front_ + 1) % mod();
        }
    }
    front_ = 0;
    back_ = 0;
}

template <typename T>
void _VecDeque<T>::reserve(size_t new_cap) {
    reserve_mod(new_cap + 1);
}

template <typename T>
void _VecDeque<T>::append(_VecDeque &other) {
    reserve(size() + other.size());
    append_unchecked(other);
}

template <typename T>
void _VecDeque<T>::append_copy(const _VecDeque &other) {
    reserve(size() + other.size());
    append_copy_unchecked(other);
}

template <typename T>
std::optional<T> _VecDeque<T>::pop_back() {
    if (!is_empty()) {
        return pop_back_unchecked();
    } else {
        return std::nullopt;
    }
}

template <typename T>
std::optional<T> _VecDeque<T>::pop_front() {
    if (!is_empty()) {
        return pop_front_unchecked();
    } else {
        return std::nullopt;
    }
}

template <typename T>
void _VecDeque<T>::push_back(T &&value) {
    if (size() == capacity()) {
        grow();
    }
    return push_back_unchecked(std::move(value));
}

template <typename T>
void _VecDeque<T>::push_front(T &&value) {
    if (size() == capacity()) {
        grow();
    }
    return push_front_unchecked(std::move(value));
}

template <typename T>
void _VecDeque<T>::push_back(const T &value) {
    return push_back(T(value));
}

template <typename T>
void _VecDeque<T>::push_front(const T &value) {
    return push_front(T(value));
}

template <typename T>
size_t _VecDeque<T>::skip_front(size_t count) {
    size_t skip = 0;
    if constexpr (std::is_trivial_v<T>) {
        if (count != 0) {
            skip = std::min(count, ((back_ + mod()) - front_) % mod());
            front_ = (front_ + skip) % mod();
        }
    } else {
        while (front_ != back_ && skip < count) {
            data_[front_].assume_init().~T();
            front_ = (front_ + 1) % mod();
            skip += 1;
        }
    }
    return skip;
}

template <typename T>
size_t _VecDeque<T>::skip_back(size_t count) {
    size_t skip = 0;
    if constexpr (std::is_trivial_v<T>) {
        if (count != 0) {
            skip = std::min(count, ((back_ + mod()) - front_) % mod());
            back_ = (back_ + mod() - skip) % mod();
        }
    } else {
        while (front_ != back_ && skip < count) {
            back_ = (back_ + mod() - 1) % mod();
            data_[back_].assume_init().~T();
            skip += 1;
        }
    }
    return skip;
}

template <typename T>
std::pair<Slice<T>, Slice<T>> _VecDeque<T>::as_slices() {
    T *data = reinterpret_cast<T *>(data_.data());
    if (front_ <= back_) {
        return std::pair{
            Slice{data + front_, back_ - front_},
            Slice<T>{},
        };
    } else {
        return std::pair{
            Slice{data + front_, mod() - front_},
            Slice{data, back_},
        };
    }
}

template <typename T>
std::pair<Slice<const T>, Slice<const T>> _VecDeque<T>::as_slices() const {
    const T *data = reinterpret_cast<const T *>(data_.data());
    if (front_ <= back_) {
        return std::pair{
            Slice{data + front_, back_ - front_},
            Slice<const T>{},
        };
    } else {
        return std::pair{
            Slice{data + front_, mod() - front_},
            Slice{data, back_},
        };
    }
}

template <typename T>
std::pair<Slice<MaybeUninit<T>>, Slice<MaybeUninit<T>>> _VecDeque<T>::free_space_as_slices() {
    if (back_ < front_) {
        return std::pair{
            Slice{&data_[back_], front_ - back_ - 1},
            Slice<MaybeUninit<T>>{},
        };
    } else {
        return std::pair{
            Slice{&data_[back_], mod() - back_ - (front_ == 0)},
            Slice{&data_[0], front_ == 0 ? 0 : front_ - 1},
        };
    }
}

template <typename T>
void _VecDeque<T>::expand_front(size_t count) {
    assert_true(count <= capacity() - size());
    front_ = ((front_ + mod()) - count) % mod();
}

template <typename T>
void _VecDeque<T>::expand_back(size_t count) {
    assert_true(count <= capacity() - size());
    back_ = (back_ + count) % mod();
}

template <typename T>
VecDequeView<T> _VecDeque<T>::view() {
    auto [first, second] = as_slices();
    return VecDequeView<T>(first, second);
}

template <typename T>
VecDequeView<const T> _VecDeque<T>::view() const {
    auto [first, second] = as_slices();
    return VecDequeView<const T>(first, second);
}

template <typename T>
size_t _VecDequeView<T>::size() const {
    return first_.size() + second_.size();
}

template <typename T>
bool _VecDequeView<T>::is_empty() const {
    return first_.size() == 0 && second_.size() == 0;
}

template <typename T>
void _VecDequeView<T>::clear() {
    *this = _VecDequeView();
}

template <typename T>
std::optional<std::reference_wrapper<T>> _VecDequeView<T>::pop_back() {
    if (!second_.is_empty()) {
        return second_.pop_back();
    } else {
        return first_.pop_back();
    }
}

template <typename T>
std::optional<std::reference_wrapper<T>> _VecDequeView<T>::pop_front() {
    auto ret = first_.pop_front();
    if (first_.is_empty() && !second_.is_empty()) {
        std::swap(first_, second_);
    }
    return ret;
}

template <typename T>
size_t _VecDequeView<T>::skip_front(size_t count) {
    size_t first_skip = first_.skip_front(count);
    size_t second_skip = 0;
    if (first_.is_empty()) {
        std::swap(first_, second_);
        second_skip = first_.skip_front(count - first_skip);
    }
    return first_skip + second_skip;
}

template <typename T>
size_t _VecDequeView<T>::skip_back(size_t count) {
    size_t second_skip = second_.skip_back(count);
    size_t first_skip = first_.skip_back(count - second_skip);
    return second_skip - first_skip;
}

template <typename T>
std::pair<Slice<T>, Slice<T>> _VecDequeView<T>::as_slices() {
    return std::pair(first_, second_);
}

template <typename T>
std::pair<Slice<const T>, Slice<const T>> _VecDequeView<T>::as_slices() const {
    return std::pair(first_, second_);
}
