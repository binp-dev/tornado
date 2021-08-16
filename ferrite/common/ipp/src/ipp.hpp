#pragma once

#include <type_traits>
#include <algorithm>
#include <vector>
#include <string>
#include <optional>
#include <variant>
#include <cassert>

#include "ipp.h"

namespace ipp {

class Msg {
public:
    virtual ~Msg() = default;

    virtual size_t length() const = 0;
    virtual void store(uint8_t *data) const = 0;
};

class MsgPrim : public virtual Msg {};

template <typename R>
class MsgState : public virtual Msg {
public:
    typedef R Raw;

    virtual R raw() const = 0;
};

template <typename Self>
class MsgEmpty : public virtual MsgPrim {
public:
    virtual size_t length() const override {
        return 0;
    }
    virtual void store(uint8_t *) const override {}

    static Self load(const uint8_t *) {
        return Self {};
    }
};

template <IppTypeApp T>
class MsgAppPrim : public virtual MsgPrim {
public:
    static constexpr IppTypeApp TYPE = T;

    virtual IppMsgAppAny raw_any() const = 0;
};

template <typename Self, IppTypeApp T>
class MsgAppEmpty : public virtual MsgAppPrim<T>, public virtual MsgEmpty<Self> {
public:
    static constexpr IppTypeApp TYPE = T;

    virtual IppMsgAppAny raw_any() const override {
        IppMsgAppAny any;
        any.type = T;
        return any;
    };

    static Self from_raw_any(const IppMsgAppAny &any) {
        assert(any.type == TYPE);
        return Self {};
    }
};

template <IppTypeMcu T>
class MsgMcuPrim : public virtual MsgPrim {
public:
    static constexpr IppTypeMcu TYPE = T;

    virtual IppMsgMcuAny raw_any() const = 0;
};

template <typename Self, IppTypeMcu T>
class MsgMcuEmpty : public virtual MsgMcuPrim<T>, public virtual MsgEmpty<Self> {
public:
    static constexpr IppTypeMcu TYPE = T;

    virtual IppMsgMcuAny raw_any() const override {
        IppMsgMcuAny any;
        any.type = T;
        return any;
    };

    static Self from_raw_any(const IppMsgMcuAny &any) {
        assert(any.type == TYPE);
        return Self {};
    }
};

class MsgAppNone final : public virtual MsgAppEmpty<MsgAppNone, IPP_APP_NONE> {};
class MsgAppStart final : public virtual MsgAppEmpty<MsgAppStart, IPP_APP_START> {};
class MsgAppStop final : public virtual MsgAppEmpty<MsgAppStop, IPP_APP_STOP> {};

class MsgAppDacSet final :
    public virtual MsgAppPrim<IPP_APP_DAC_SET>,
    public virtual MsgState<_IppMsgAppDacSet>
{
private:
    uint32_t value_;

public:
    inline explicit MsgAppDacSet(uint32_t value) : value_(value) {}

    inline virtual Raw raw() const override {
        return Raw { value_ };
    }
    inline virtual IppMsgAppAny raw_any() const override {
        IppMsgAppAny any;
        any.type = TYPE;
        any.dac_set = this->raw();
        return any;
    }

    inline virtual size_t length() const override {
        return _IPP_MSG_APP_DAC_SET_LEN;
    }
    inline virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_app_store_dac_set(&raw, data);
    }

    inline static MsgAppDacSet from_raw(const Raw &raw) {
        return MsgAppDacSet(raw.value);
    }
    inline static MsgAppDacSet from_raw_any(const IppMsgAppAny &any) {
        assert(any.type == TYPE);
        return from_raw(any.dac_set);
    }

    inline static std::variant<MsgAppDacSet, IppLoadStatus> load(uint8_t *data, size_t max_length) {
        _IppMsgAppDacSet dst;
        const auto status = _ipp_msg_app_load_dac_set(&dst, data, max_length);
        if (IPP_LOAD_OK != status) {
            return status;
        }
        return from_raw(dst);
    }

    inline uint32_t value() const {
        return this->value_;
    }
    inline void set_value(uint32_t value) {
        this->value_ = value_;
    }
};

class MsgAppAdcReq final :
    public virtual MsgAppPrim<IPP_APP_ADC_REQ>,
    public virtual MsgState<_IppMsgAppAdcReq>
{
private:
    uint8_t index_;

public:
    inline explicit MsgAppAdcReq(uint8_t index) : index_(index) {}

    inline virtual Raw raw() const override {
        return Raw { index_ };
    }
    inline virtual IppMsgAppAny raw_any() const override {
        IppMsgAppAny any;
        any.type = TYPE;
        any.adc_req = this->raw();
        return any;
    }

    inline virtual size_t length() const override {
        return _IPP_MSG_APP_ADC_REQ_LEN;
    }
    inline virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_app_store_adc_req(&raw, data);
    }

    inline static MsgAppAdcReq from_raw(const Raw &raw) {
        return MsgAppAdcReq(raw.index);
    }
    inline static MsgAppAdcReq from_raw_any(const IppMsgAppAny &any) {
        assert(any.type == TYPE);
        return from_raw(any.adc_req);
    }

    inline static std::variant<MsgAppAdcReq, IppLoadStatus> load(uint8_t *data, size_t max_length) {
        _IppMsgAppAdcReq dst;
        const auto status = _ipp_msg_app_load_adc_req(&dst, data, max_length);
        if (IPP_LOAD_OK != status) {
            return status;
        }
        return from_raw(dst);
    }

    inline uint8_t index() const {
        return this->index_;
    }
    inline void set_index(uint8_t index) {
        this->index_ = index_;
    }
};

class MsgMcuNone final : public MsgMcuEmpty<MsgMcuNone, IPP_MCU_NONE> {};


class MsgMcuAdcVal final :
    public virtual MsgMcuPrim<IPP_MCU_ADC_VAL>,
    public virtual MsgState<_IppMsgMcuAdcVal>
{
private:
    uint8_t index_;
    uint32_t value_;

public:
    inline MsgMcuAdcVal(uint8_t index, uint32_t value) : index_(index), value_(value) {}

    inline virtual Raw raw() const override {
        return Raw { index_, value_ };
    }
    inline virtual IppMsgMcuAny raw_any() const override {
        IppMsgMcuAny any;
        any.type = TYPE;
        any.adc_val = this->raw();
        return any;
    }

    inline virtual size_t length() const override {
        return _IPP_MSG_MCU_ADC_VAL_LEN;
    }
    inline virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_mcu_store_adc_val(&raw, data);
    }

    inline static MsgMcuAdcVal from_raw(const Raw &raw) {
        return MsgMcuAdcVal(raw.index, raw.value);
    }
    inline static MsgMcuAdcVal from_raw_any(const IppMsgMcuAny &any) {
        assert(any.type == TYPE);
        return from_raw(any.adc_val);
    }

    inline static std::variant<MsgMcuAdcVal, IppLoadStatus> load(uint8_t *data, size_t max_length) {
        _IppMsgMcuAdcVal dst;
        const auto status = _ipp_msg_mcu_load_adc_val(&dst, data, max_length);
        if (IPP_LOAD_OK != status) {
            return status;
        }
        return from_raw(dst);
    }

    inline uint8_t index() const {
        return this->index_;
    }
    inline uint32_t value() const {
        return this->value_;
    }
};

class MsgMcuError final :
    public virtual MsgMcuPrim<IPP_MCU_ERROR>,
    public virtual MsgState<_IppMsgMcuError>
{
private:
    uint8_t code_;
    std::string message_;

public:
    inline MsgMcuError(uint8_t code, std::string &&message) : code_(code), message_(std::move(message)) {}
    inline MsgMcuError(uint8_t code, const char *message) : code_(code), message_(message) {}

    inline virtual Raw raw() const override {
        return Raw { code_, message_.c_str() };
    }
    inline virtual IppMsgMcuAny raw_any() const override {
        IppMsgMcuAny any;
        any.type = TYPE;
        any.error = this->raw();
        return any;
    }

    inline virtual size_t length() const override {
        const auto raw = this->raw();
        return _ipp_msg_mcu_len_error(&raw);
    }
    inline virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_mcu_store_error(&raw, data);
    }

    inline static MsgMcuError from_raw(const Raw &raw) {
        return MsgMcuError(raw.code, std::string(raw.message));
    }
    inline static MsgMcuError from_raw_any(const IppMsgMcuAny &any) {
        assert(any.type == TYPE);
        return from_raw(any.error);
    }

    inline static std::variant<MsgMcuError, IppLoadStatus> load(uint8_t *data, size_t max_length) {
        _IppMsgMcuError dst;
        const auto status = _ipp_msg_mcu_load_error(&dst, data, max_length);
        if (IPP_LOAD_OK != status) {
            return status;
        }
        return from_raw(dst);
    }

    inline uint8_t code() const {
        return this->code_;
    }
    inline const std::string &message() const {
        return this->message_;
    }
    inline std::string &message() {
        return this->message_;
    }
};

class MsgMcuDebug final :
    public virtual MsgMcuPrim<IPP_MCU_DEBUG>,
    public virtual MsgState<_IppMsgMcuDebug>
{
private:
    std::string message_;

public:
    MsgMcuDebug(std::string &&message) : message_(std::move(message)) {}
    MsgMcuDebug(const char *message) : message_(message) {}

    inline virtual Raw raw() const override {
        return Raw { message_.c_str() };
    }
    inline virtual IppMsgMcuAny raw_any() const override {
        IppMsgMcuAny any;
        any.type = TYPE;
        any.debug = this->raw();
        return any;
    }

    inline virtual size_t length() const override {
        const auto raw = this->raw();
        return _ipp_msg_mcu_len_debug(&raw);
    }
    inline virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_mcu_store_debug(&raw, data);
    }

    inline static MsgMcuDebug from_raw(const Raw &raw) {
        return MsgMcuDebug(std::string(raw.message));
    }
    inline static MsgMcuDebug from_raw_any(const IppMsgMcuAny &any) {
        assert(any.type == TYPE);
        return from_raw(any.debug);
    }

    inline static std::variant<MsgMcuDebug, IppLoadStatus> load(uint8_t *data, size_t max_length) {
        _IppMsgMcuDebug dst;
        const auto status = _ipp_msg_mcu_load_debug(&dst, data, max_length);
        if (IPP_LOAD_OK != status) {
            return status;
        }
        return from_raw(dst);
    }

    inline const std::string &message() const {
        return this->message_;
    }
    inline std::string &message() {
        return this->message_;
    }
};

template <typename A, typename K, typename ...Vs>
class MsgAny : public virtual MsgState<A> {
public:
    typedef std::variant<Vs...> Variant;
    typedef typename MsgState<A>::Raw Raw;

private:
    Variant variant_;

public:
    explicit MsgAny(Variant &&variant) : variant_(std::move(variant)) {}

    const auto &variant() const {
        return this->variant_;
    }
    auto &variant() {
        return this->variant_;
    }

    virtual Raw raw() const override {
        return std::visit([&](const auto &inner) {
            return inner.raw_any();
        }, this->variant());
    }

    static std::variant<Vs...> variant_from_raw(const Raw &raw) {
        static constexpr size_t N = sizeof...(Vs);
        static constexpr K ids[] = { Vs::TYPE... };
        const size_t i = std::find_if(ids, ids + N, [&](K t) { return t == raw.type; }) - ids;
        static constexpr std::variant<Vs*...> types[] = { (Vs*)nullptr... };
        return std::visit([&](auto *ptr) {
            return std::variant<Vs...>(
                std::remove_reference_t<decltype(*ptr)>::from_raw_any(raw)
            );
        }, types[i]);
    }
};

namespace detail {
using MsgAppAnyBase = MsgAny<
    IppMsgAppAny,
    IppTypeApp,
    // Variants
    MsgAppNone,
    MsgAppStart,
    MsgAppStop,
    MsgAppDacSet,
    MsgAppAdcReq
>;
}

class MsgAppAny final : public virtual detail::MsgAppAnyBase {
public:
    inline explicit MsgAppAny(Variant &&variant) : MsgAny(std::move(variant)) {}

    inline MsgAppAny(MsgAppAny &&other) : MsgAny(std::move(other.variant())) {}
    inline MsgAppAny &operator=(MsgAppAny &&other) {
        this->variant() = std::move(other.variant());
        return *this;
    }

    inline virtual size_t length() const override {
        return std::visit([&](const auto &inner) {
            const IppMsgAppAny any = inner.raw_any();
            return ipp_msg_app_len(&any);
        }, this->variant());
    }
    inline virtual void store(uint8_t *data) const override {
        std::visit([&](const auto &inner) {
            const IppMsgAppAny any = inner.raw_any();
            ipp_msg_app_store(&any, data);
        }, this->variant());
    }

    inline static std::variant<MsgAppAny, IppLoadStatus> load(uint8_t *data, size_t max_length) {
        IppMsgAppAny dst;
        const auto status = ipp_msg_app_load(&dst, data, max_length);
        if (IPP_LOAD_OK != status) {
            return status;
        }
        return MsgAppAny(variant_from_raw(dst));
    }
};

namespace detail {
using MsgMcuAnyBase = MsgAny<
    IppMsgMcuAny,
    IppTypeMcu,
    // Variants
    MsgMcuNone,
    MsgMcuAdcVal,
    MsgMcuError,
    MsgMcuDebug
>;
}

class MsgMcuAny final : public virtual detail::MsgMcuAnyBase {
public:
    inline explicit MsgMcuAny(Variant &&variant) : MsgAny(std::move(variant)) {}

    inline MsgMcuAny(MsgMcuAny &&other) : MsgAny(std::move(other.variant())) {}
    inline MsgMcuAny &operator=(MsgMcuAny &&other) {
        this->variant() = std::move(other.variant());
        return *this;
    }

    inline virtual size_t length() const override {
        return std::visit([&](const auto &inner) {
            const IppMsgMcuAny any = inner.raw_any();
            return ipp_msg_mcu_len(&any);
        }, this->variant());
    }
    inline virtual void store(uint8_t *data) const override {
        std::visit([&](const auto &inner) {
            const IppMsgMcuAny any = inner.raw_any();
            ipp_msg_mcu_store(&any, data);
        }, this->variant());
    }

    inline static std::variant<MsgMcuAny, IppLoadStatus> load(uint8_t *data, size_t max_length) {
        IppMsgMcuAny dst;
        const auto status = ipp_msg_mcu_load(&dst, data, max_length);
        if (IPP_LOAD_OK != status) {
            return status;
        }
        return MsgMcuAny(variant_from_raw(dst));
    }
};

} // namespace ipp
