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

    virtual size_t size() const = 0;
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
    virtual size_t size() const override {
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

class MsgAppWfData final :
    public virtual MsgAppPrim<IPP_APP_WF_DATA>,
    public virtual MsgState<_IppMsgAppWfData>
{
private:
    std::vector<uint8_t> data_;

public:
    inline explicit MsgAppWfData(std::vector<uint8_t> &&data) : data_(std::move(data)) {}

    inline virtual Raw raw() const override {
        return Raw { data_.data(), data_.size() };
    }
    inline virtual IppMsgAppAny raw_any() const override {
        IppMsgAppAny any;
        any.type = TYPE;
        any.wf_data = this->raw();
        return any;
    }

    inline virtual size_t size() const override {
        const auto raw = this->raw();
        return _ipp_msg_app_len_wf_data(&raw);
    }
    inline virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_app_store_wf_data(&raw, data);
    }

    inline static MsgAppWfData from_raw(const Raw &raw) {
        return MsgAppWfData(std::vector<uint8_t>(raw.data, raw.data + raw.len));
    }
    inline static MsgAppWfData from_raw_any(const IppMsgAppAny &any) {
        assert(any.type == TYPE);
        return from_raw(any.wf_data);
    }

    inline static MsgAppWfData load(uint8_t *data) {
        return from_raw(_ipp_msg_app_load_wf_data(data));
    }

    inline const std::vector<uint8_t> &data() const {
        return this->data_;
    }
};

class MsgMcuNone final : public MsgMcuEmpty<MsgMcuNone, IPP_MCU_NONE> {};
class MsgMcuWfReq final : public MsgMcuEmpty<MsgMcuWfReq, IPP_MCU_WF_REQ> {};

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

    inline virtual size_t size() const override {
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

    inline static MsgMcuError load(uint8_t *data) {
        return from_raw(_ipp_msg_mcu_load_error(data));
    }

    inline uint8_t code() const {
        return this->code_;
    }
    inline const std::string &message() const {
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

    inline virtual size_t size() const override {
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

    inline static MsgMcuDebug load(uint8_t *data) {
        return from_raw(_ipp_msg_mcu_load_debug(data));
    }

    inline const std::string &message() const {
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

class MsgAppAny final : public virtual MsgAny<
    IppMsgAppAny,
    IppTypeApp,
    // Variants
    MsgAppNone,
    MsgAppStart,
    MsgAppStop,
    MsgAppWfData
> {
public:
    inline explicit MsgAppAny(Variant &&variant) : MsgAny(std::move(variant)) {}

    inline virtual size_t size() const override {
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

    inline static MsgAppAny load(uint8_t *data) {
        return MsgAppAny(variant_from_raw(ipp_msg_app_load(data)));
    }
};

class MsgMcuAny final : public virtual MsgAny<
    IppMsgMcuAny,
    IppTypeMcu,
    // Variants
    MsgMcuNone,
    MsgMcuWfReq,
    MsgMcuError,
    MsgMcuDebug
> {
public:
    inline explicit MsgMcuAny(Variant &&variant) : MsgAny(std::move(variant)) {}

    inline virtual size_t size() const override {
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

    inline static MsgMcuAny load(uint8_t *data) {
        return MsgMcuAny(variant_from_raw(ipp_msg_mcu_load(data)));
    }
};

} // namespace ipp