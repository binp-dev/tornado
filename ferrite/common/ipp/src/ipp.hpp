#pragma once

#include <variant>

#include "ipp.h"

namespace ipp {

template <typename R = void>
class Msg {
private:
    R raw_;

public:
    Msg(R raw) : raw(raw) {}
    virtual ~Msg() = default;

    const R &raw() const {
        this->raw;
    }

    virtual size_t size() const = 0;
    virtual void store(uint8_t *data) const = 0;
};

class Msg<void> {
public:
    virtual ~Msg() = default;

    virtual size_t size() const {
        return 0;
    }
    virtual void store(uint8_t *data) const {}
};

template <typename R = void>
class MsgApp : public Msg<R> {};
template <typename R = void>
class MsgMcu : public Msg<R> {};

class MsgAppNone final : public MsgApp<> {
protected:
    static const IppTypeApp TYPE = IPP_APP_NONE;
};
class MsgAppStart final : public MsgApp<> {
protected:
    static const IppTypeApp TYPE = IPP_APP_START;
};
class MsgAppStop final : public MsgApp<> {
protected:
    static const IppTypeApp TYPE = IPP_APP_STOP;
};
class MsgAppWfData final : public MsgApp<_IppMsgAppWfData> {
public:
    static const IppTypeApp TYPE = IPP_APP_WF_DATA;

    virtual size_t size() const override {
        return _ipp_msg_app_len_wf_data(&this->raw());
    }
    virtual void store(uint8_t *data) const override {
        _ipp_msg_app_store_wf_data(&this->raw(), data);
    }
};

class MsgMcuNone final : public MsgMcu<> {
    static const IppTypeMcu TYPE = IPP_MCU_NONE;
};
class MsgMcuWfReq final : public MsgMcu<> {
    static const IppTypeMcu TYPE = IPP_MCU_WF_REQ;
};
class MsgMcuError final : public MsgMcu<_IppMsgMcuError> {
    static const IppTypeMcu TYPE = IPP_MCU_ERROR;

    virtual size_t size() const override {
        return _ipp_msg_mcu_len_error(&this->raw());
    }
    virtual void store(uint8_t *data) const override {
        _ipp_msg_mcu_store_error(&this->raw(), data);
    }
};
class MsgMcuDebug final : public MsgMcu<_IppMsgMcuDebug> {
    static const IppTypeMcu TYPE = IPP_MCU_DEBUG;

    virtual size_t size() const override {
        return _ipp_msg_mcu_len_debug(&this->raw());
    }
    virtual void store(uint8_t *data) const override {
        _ipp_msg_mcu_store_debug(&this->raw(), data);
    }
};

class MsgAppAny final : public MsgApp<IppMsgAppAny> {
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
        return std::visit([](const auto &&inner) {
            return 1 + inner.raw();
        }, this->variant());
    }
    virtual void store(uint8_t *data) const override {
        std::visit([](const auto &&inner) {
            data[0] = (uint8_t)decltype(inner)::TYPE;
            inner.store(data + 1);
        }, this->variant());
    }
};

class MsgMcuAny final : public MsgMcu<IppMsgMcuAny> {
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
        return std::visit([](const auto &&inner) {
            return 1 + inner.raw();
        }, this->variant());
    }
    virtual void store(uint8_t *data) const override {
        std::visit([](const auto &&inner) {
            data[0] = (uint8_t)decltype(inner)::TYPE;
            inner.store(data + 1);
        }, this->variant());
    }
};

} // namespace ipp
