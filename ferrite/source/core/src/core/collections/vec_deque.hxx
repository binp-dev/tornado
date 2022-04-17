#include "vec_deque.hpp"

namespace core::_impl {

template <typename T>
BasicVecDeque<T>::BasicVecDeque(const BasicVecDeque &other) : BasicVecDeque(other.size()) {
    append_copy_unchecked(other);
}

template <typename T>
BasicVecDeque<T> &BasicVecDeque<T>::operator=(const BasicVecDeque<T> &other) {
    clear();
    append_copy(other);
    return *this;
}

template <typename T>
BasicVecDeque<T>::BasicVecDeque(BasicVecDeque &&other) :
    data_(std::move(other.data_)),
    front_(other.front_),
    back_(other.back_) {
    other.front_ = 0;
    other.back_ = 0;
}

template <typename T>
BasicVecDeque<T> &BasicVecDeque<T>::operator=(BasicVecDeque<T> &&other) {
    clear();

    data_ = std::move(other.data_);
    front_ = other.front_;
    back_ = other.back_;

    other.front_ = 0;
    other.back_ = 0;

    return *this;
}

template <typename T>
size_t BasicVecDeque<T>::capacity() const {
    if (mod() > 1) {
        return mod() - 1;
    } else {
        return 0;
    }
}

template <typename T>
size_t BasicVecDeque<T>::size() const {
    if (mod() == 0) {
        return 0;
    } else {
        return ((back_ + mod()) - front_) % mod();
    }
}

template <typename T>
bool BasicVecDeque<T>::empty() const {
    return size() == 0;
}

template <typename T>
T BasicVecDeque<T>::pop_back_unchecked() {
    size_t new_back = (back_ + mod() - 1) % mod();
    T &ref = data_[new_back].assume_init();
    T val(std::move(ref));
    back_ = new_back;
    ref.~T();
    return val;
}

template <typename T>
T BasicVecDeque<T>::pop_front_unchecked() {
    size_t new_front = (front_ + 1) % mod();
    T &ref = data_[front_].assume_init();
    T val(std::move(ref));
    front_ = new_front;
    ref.~T();
    return val;
}

template <typename T>
void BasicVecDeque<T>::push_back_unchecked(T &&value) {
    size_t new_back = (back_ + 1) % mod();
    data_[back_].init_in_place(std::move(value));
    back_ = new_back;
}

template <typename T>
void BasicVecDeque<T>::push_front_unchecked(T &&value) {
    size_t new_front = (front_ + mod() - 1) % mod();
    data_[new_front].init_in_place(std::move(value));
    front_ = new_front;
}

template <typename T>
void BasicVecDeque<T>::append_unchecked(BasicVecDeque<T> &other) {
    while (other.front_ != other.back_) {
        data_[back_].init_in_place(std::move(other.data_[other.front_].assume_init()));
        other.front_ = (other.front_ + 1) % other.mod();
        back_ = (back_ + 1) % mod();
    }
}

template <typename T>
void BasicVecDeque<T>::append_copy_unchecked(const BasicVecDeque<T> &other) {
    size_t front_view = other.front_;
    while (front_view != other.back_) {
        data_[back_].init_in_place(other.data_[front_view].assume_init());
        front_view = (front_view + 1) % other.mod();
        back_ = (back_ + 1) % mod();
    }
}

template <typename T>
void BasicVecDeque<T>::reserve_mod(size_t new_mod) {
    if (new_mod > std::max(size_t(1), mod())) {
        BasicVecDeque<T> new_self(new_mod - 1);
        new_self.append_unchecked(*this);
        *this = std::move(new_self);
    }
}

template <typename T>
void BasicVecDeque<T>::grow() {
    if (mod() > 1) {
        reserve_mod(2 * mod());
    } else {
        reserve_mod(2);
    }
}

template <typename T>
void BasicVecDeque<T>::grow_to_free(size_t count) {
    size_t new_mod = std::max(mod(), size_t(2));
    while (new_mod < size() + count + 1) {
        new_mod = 2 * new_mod;
    }
    reserve_mod(new_mod);
}


template <typename T>
void BasicVecDeque<T>::clear() {
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
void BasicVecDeque<T>::reserve(size_t new_cap) {
    reserve_mod(new_cap + 1);
}

template <typename T>
void BasicVecDeque<T>::append(BasicVecDeque &other) {
    reserve(size() + other.size());
    append_unchecked(other);
}

template <typename T>
void BasicVecDeque<T>::append_copy(const BasicVecDeque &other) {
    reserve(size() + other.size());
    append_copy_unchecked(other);
}

template <typename T>
std::optional<T> BasicVecDeque<T>::pop_back() {
    if (!empty()) {
        return pop_back_unchecked();
    } else {
        return std::nullopt;
    }
}

template <typename T>
std::optional<T> BasicVecDeque<T>::pop_front() {
    if (!empty()) {
        return pop_front_unchecked();
    } else {
        return std::nullopt;
    }
}

template <typename T>
void BasicVecDeque<T>::push_back(T &&value) {
    if (size() == capacity()) {
        grow();
    }
    return push_back_unchecked(std::move(value));
}

template <typename T>
void BasicVecDeque<T>::push_front(T &&value) {
    if (size() == capacity()) {
        grow();
    }
    return push_front_unchecked(std::move(value));
}

template <typename T>
void BasicVecDeque<T>::push_back(const T &value) {
    return push_back(T(value));
}

template <typename T>
void BasicVecDeque<T>::push_front(const T &value) {
    return push_front(T(value));
}

template <typename T>
size_t BasicVecDeque<T>::skip_front(size_t count) {
    size_t skip = 0;
    if constexpr (std::is_trivial_v<T>) {
        if (count != 0) {
            skip = std::min(count, this->size());
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
size_t BasicVecDeque<T>::skip_back(size_t count) {
    size_t skip = 0;
    if constexpr (std::is_trivial_v<T>) {
        if (count != 0) {
            skip = std::min(count, this->size());
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
std::pair<Slice<T>, Slice<T>> BasicVecDeque<T>::as_slices() {
    T *data = reinterpret_cast<T *>(data_.data());
    if (front_ <= back_) {
        return std::pair{
            Slice<T>{data + front_, back_ - front_},
            Slice<T>{},
        };
    } else {
        return std::pair{
            Slice<T>{data + front_, mod() - front_},
            Slice<T>{data, back_},
        };
    }
}

template <typename T>
std::pair<Slice<const T>, Slice<const T>> BasicVecDeque<T>::as_slices() const {
    const T *data = reinterpret_cast<const T *>(data_.data());
    if (front_ <= back_) {
        return std::pair{
            Slice<const T>{data + front_, back_ - front_},
            Slice<const T>{},
        };
    } else {
        return std::pair{
            Slice<const T>{data + front_, mod() - front_},
            Slice<const T>{data, back_},
        };
    }
}

template <typename T>
std::pair<Slice<MaybeUninit<T>>, Slice<MaybeUninit<T>>> BasicVecDeque<T>::free_space_as_slices() {
    if (back_ < front_) {
        return std::pair{
            Slice<MaybeUninit<T>>{&data_[back_], front_ - back_ - 1},
            Slice<MaybeUninit<T>>{},
        };
    } else {
        return std::pair{
            Slice<MaybeUninit<T>>{&data_[back_], mod() - back_ - (front_ == 0)},
            Slice<MaybeUninit<T>>{&data_[0], front_ == 0 ? 0 : front_ - 1},
        };
    }
}

template <typename T>
void BasicVecDeque<T>::expand_front(size_t count) {
    core_assert(count <= capacity() - size());
    front_ = ((front_ + mod()) - count) % mod();
}

template <typename T>
void BasicVecDeque<T>::expand_back(size_t count) {
    core_assert(count <= capacity() - size());
    back_ = (back_ + count) % mod();
}

template <typename T>
VecDequeView<T> BasicVecDeque<T>::view() {
    auto [first, second] = as_slices();
    return VecDequeView<T>(first, second);
}

template <typename T>
VecDequeView<const T> BasicVecDeque<T>::view() const {
    auto [first, second] = as_slices();
    return VecDequeView<const T>(first, second);
}

template <typename T>
size_t BasicVecDequeView<T>::size() const {
    return first_.size() + second_.size();
}

template <typename T>
bool BasicVecDequeView<T>::empty() const {
    return first_.size() == 0;
}

template <typename T>
void BasicVecDequeView<T>::clear() {
    *this = BasicVecDequeView();
}

template <typename T>
std::optional<std::reference_wrapper<T>> BasicVecDequeView<T>::pop_back() {
    if (!second_.empty()) {
        return second_.pop_back();
    } else {
        return first_.pop_back();
    }
}

template <typename T>
std::optional<std::reference_wrapper<T>> BasicVecDequeView<T>::pop_front() {
    auto ret = first_.pop_front();
    if (first_.empty() && !second_.empty()) {
        std::swap(first_, second_);
    }
    return ret;
}

template <typename T>
size_t BasicVecDequeView<T>::skip_front(size_t count) {
    size_t first_skip = first_.skip_front(count);
    size_t second_skip = 0;
    if (first_.empty()) {
        std::swap(first_, second_);
        second_skip = first_.skip_front(count - first_skip);
    }
    return first_skip + second_skip;
}

template <typename T>
size_t BasicVecDequeView<T>::skip_back(size_t count) {
    size_t second_skip = second_.skip_back(count);
    size_t first_skip = first_.skip_back(count - second_skip);
    return second_skip + first_skip;
}

template <typename T>
std::pair<Slice<T>, Slice<T>> BasicVecDequeView<T>::as_slices() {
    return std::pair(first_, second_);
}

template <typename T>
std::pair<Slice<const T>, Slice<const T>> BasicVecDequeView<T>::as_slices() const {
    return std::pair(first_, second_);
}


template <typename T>
size_t StreamVecDeque<T, true>::read_array(std::span<T> data) {
    size_t read_len = this->view().read_array(data);
    core_assert_eq(this->skip_front(read_len), read_len);
    return read_len;
}

template <typename T>
bool StreamVecDeque<T, true>::read_array_exact(std::span<T> data) {
    if (this->view().read_array_exact(data)) {
        core_assert_eq(this->skip_front(data.size()), data.size());
        return true;
    } else {
        return false;
    }
}

template <typename T>
size_t StreamVecDeque<T, true>::write_array(std::span<const T> data) {
    // Reserve enough space for new elements.
    this->grow_to_free(data.size());

    // Copy data to queue.
    auto [left, right] = this->free_space_as_slices();
    size_t left_len = std::min(left.size(), data.size());
    memcpy(left.data(), data.data(), sizeof(T) * left_len);
    size_t right_len = std::min(right.size(), data.size() - left_len);
    memcpy(right.data(), data.data() + left_len, sizeof(T) * right_len);

    core_assert_eq(left_len + right_len, data.size());
    this->expand_back(data.size());
    return data.size();
}

template <typename T>
bool StreamVecDeque<T, true>::write_array_exact(std::span<const T> data) {
    core_assert_eq(this->write_array(data), data.size());
    return true;
}

template <typename T>
size_t StreamVecDeque<T, true>::write_array_from(ReadArray<T> &stream, std::optional<size_t> len_opt) {
    if (len_opt.has_value()) {
        size_t len = len_opt.value();

        // Reserve enough space for new elements.
        this->grow_to_free(len);

        // Read first slice.
        auto [left, right] = this->free_space_as_slices();
        size_t left_len = std::min(left.size(), len);
        size_t left_res_len = stream.read_array(std::span(reinterpret_cast<T *>(left.data()), left_len));
        this->expand_back(left_res_len);
        if (left_res_len < left_len) {
            return left_res_len;
        }

        // Read second slice.
        size_t right_len = std::min(right.size(), len - left_len);
        auto right_res_len = stream.read_array(std::span(reinterpret_cast<T *>(right.data()), right_len));
        this->expand_back(right_res_len);
        return left_len + right_res_len;
    } else {
        // Read infinitely until stream ends.
        size_t total = 0;
        for (;;) {
            size_t free = this->capacity() - this->size();
            if (free > 0) {
                size_t res_len = write_array_from(stream, free);
                total += res_len;
                if (res_len < free) {
                    return total;
                }
            }
            this->grow();
        }
    }
}

template <typename T>
size_t StreamVecDeque<T, true>::read_array_into(WriteArray<T> &stream, std::optional<size_t> len_opt) {
    size_t res_len = this->view().read_array_into(stream, len_opt);
    core_assert_eq(this->skip_front(res_len), res_len);
    return res_len;
}

template <typename T>
size_t StreamVecDequeView<T, true>::read_array(std::span<T> data) {
    auto [first, second] = this->as_slices();
    size_t first_len = first.read_array(data);
    size_t second_len = second.read_array(data.subspan(first_len));
    size_t read_len = first_len + second_len;
    core_assert_eq(this->skip_front(read_len), read_len);
    return read_len;
}

template <typename T>
bool StreamVecDequeView<T, true>::read_array_exact(std::span<T> data) {
    if (data.size() <= this->size()) {
        core_assert_eq(this->read_array(data), data.size());
        return true;
    } else {
        return false;
    }
}

template <typename T>
size_t StreamVecDequeView<T, true>::read_array_into(WriteArray<T> &stream, std::optional<size_t> len_opt) {
    size_t len = len_opt.value_or(this->size());
    auto [left, right] = this->as_slices();

    // Write first slice.
    size_t left_len = std::min(left.size(), len);
    size_t left_res_len = stream.write_array(left.subspan(0, left_len));
    core_assert_eq(this->skip_front(left_res_len), left_res_len);
    if (left_res_len < left_len) {
        return left_res_len;
    }

    // Write second slice.
    size_t right_len = std::min(right.size(), len - left_len);
    size_t right_res_len = stream.write_array(right.subspan(0, right_len));
    core_assert_eq(this->skip_front(right_res_len), right_res_len);
    return left_len + right_res_len;
}

} // namespace core::_impl
