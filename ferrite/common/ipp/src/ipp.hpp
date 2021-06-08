#pragma once

#include <vector>
#include <string>
#include <variant>

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

class MsgEmpty : public virtual MsgPrim {
public:
    virtual size_t size() const override {
        return 0;
    }
    virtual void store(uint8_t *) const override {}
};

template <IppTypeApp T>
class MsgAppPrim : public virtual MsgPrim {
public:
    static constexpr IppTypeApp TYPE = T;

    virtual IppMsgAppAny raw_any() const = 0;
};

template <IppTypeApp T>
class MsgAppEmpty : public virtual MsgAppPrim<T>, public virtual MsgEmpty {
    virtual IppMsgAppAny raw_any() const override {
        return IppMsgAppAny { T };
    };
};

template <IppTypeMcu T>
class MsgMcuPrim : public virtual MsgPrim {
public:
    static constexpr IppTypeMcu TYPE = T;

    virtual IppMsgMcuAny raw_any() const = 0;
};

template <IppTypeMcu T>
class MsgMcuEmpty : public virtual MsgMcuPrim<T>, public virtual MsgEmpty {
    virtual IppMsgMcuAny raw_any() const override {
        return IppMsgMcuAny { T };
    };
};

class MsgAppNone final : public virtual MsgAppEmpty<IPP_APP_NONE> {};
class MsgAppStart final : public virtual MsgAppEmpty<IPP_APP_START> {};
class MsgAppStop final : public virtual MsgAppEmpty<IPP_APP_STOP> {};

class MsgAppWfData final :
    public virtual MsgAppPrim<IPP_APP_WF_DATA>,
    public virtual MsgState<_IppMsgAppWfData>
{
private:
    std::vector<uint8_t> data;

public:
    MsgAppWfData(std::vector<uint8_t> &&data) : data(std::move(data)) {}

    virtual Raw raw() const override {
        return Raw { data.data(), data.size() };
    }
    virtual IppMsgAppAny raw_any() const override {
        IppMsgAppAny any;
        any.type = TYPE;
        any.wf_data = this->raw();
        return any;
    }

    virtual size_t size() const override {
        const auto raw = this->raw();
        return _ipp_msg_app_len_wf_data(&raw);
    }
    virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_app_store_wf_data(&raw, data);
    }
};

class MsgMcuNone final : public MsgMcuEmpty<IPP_MCU_NONE> {};
class MsgMcuWfReq final : public MsgMcuEmpty<IPP_MCU_WF_REQ> {};

class MsgMcuError final :
    public virtual MsgMcuPrim<IPP_MCU_ERROR>,
    public virtual MsgState<_IppMsgMcuError>
{
private:
    uint8_t code;
    std::string message;

public:
    MsgMcuError(uint8_t code, std::string &&message) : code(code), message(std::move(message)) {}
    MsgMcuError(uint8_t code, const char *message) : code(code), message(message) {}

    virtual Raw raw() const override {
        return Raw { code, message.c_str() };
    }
    virtual IppMsgMcuAny raw_any() const override {
        IppMsgMcuAny any;
        any.type = TYPE;
        any.error = this->raw();
        return any;
    }

    virtual size_t size() const override {
        const auto raw = this->raw();
        return _ipp_msg_mcu_len_error(&raw);
    }
    virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_mcu_store_error(&raw, data);
    }
};


class MsgMcuDebug final :
    public virtual MsgMcuPrim<IPP_MCU_DEBUG>,
    public virtual MsgState<_IppMsgMcuDebug>
{
private:
    std::string message;

public:
    MsgMcuDebug(std::string &&message) : message(std::move(message)) {}
    MsgMcuDebug(const char *message) : message(message) {}

    virtual Raw raw() const override {
        return Raw { message.c_str() };
    }
    virtual IppMsgMcuAny raw_any() const override {
        IppMsgMcuAny any;
        any.type = TYPE;
        any.debug = this->raw();
        return any;
    }

    virtual size_t size() const override {
        const auto raw = this->raw();
        return _ipp_msg_mcu_len_debug(&raw);
    }
    virtual void store(uint8_t *data) const override {
        const auto raw = this->raw();
        _ipp_msg_mcu_store_debug(&raw, data);
    }
};

class MsgAppAny final : public virtual MsgState<IppMsgAppAny> {
private:
    std::variant<
        MsgAppNone,
        MsgAppStart,
        MsgAppStop,
        MsgAppWfData
    > variant_;

public:
    const auto &variant() const {
        return this->variant_;
    }

    virtual size_t size() const override {
        return std::visit([&](const auto &&inner) {
            const auto any = inner.raw_any();
            return ipp_msg_app_len(&any);
        }, this->variant());
    }
    virtual void store(uint8_t *data) const override {
        std::visit([&](const auto &&inner) {
            const auto any = inner.raw_any();
            ipp_msg_app_store(&any, data);
        }, this->variant());
    }
};

class MsgMcuAny final : public virtual MsgState<IppMsgMcuAny> {
private:
    std::variant<
        MsgMcuNone,
        MsgMcuWfReq,
        MsgMcuError,
        MsgMcuDebug
    > variant_;

public:
    const auto &variant() const {
        return this->variant_;
    }

    virtual size_t size() const override {
        return std::visit([&](const auto &&inner) {
            const auto any = inner.raw_any();
            return ipp_msg_mcu_len(&any);
        }, this->variant());
    }
    virtual void store(uint8_t *data) const override {
        std::visit([&](const auto &&inner) {
            const auto any = inner.raw_any();
            ipp_msg_mcu_store(&any, data);
        }, this->variant());
    }
};

} // namespace ipp
