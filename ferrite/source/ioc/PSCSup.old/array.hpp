#pragma once

#include <cstdint>
#include <cstring>
#include <utility>
#include <span>

#include <dbAccess.h>
#include <epicsTypes.h>

#include <core/assert.hpp>
#include <record/array.hpp>

#include "base.hpp"
#include "types.hpp"

template <typename T, typename R, typename H>
class EpicsArrayRecordBase : public EpicsRecord<R>, public virtual HandledRecord<H> {
public:
    explicit EpicsArrayRecordBase(R *raw) : EpicsRecord<R>(raw) {
        core_assert_eq(epics_type_enum<T>, data_type());
        core_assert_eq(dbValueSize(data_type()), sizeof(T));
    }

protected:
    menuFtype data_type() const {
        return static_cast<menuFtype>(this->raw()->ftvl);
    }
};

template <typename T, typename R>
class EpicsInputArrayRecord : public virtual InputArrayRecord<T>, public EpicsArrayRecordBase<T, R, InputArrayHandler<T>> {
public:
    using EpicsArrayRecordBase<T, R, InputArrayHandler<T>>::EpicsArrayRecordBase;

protected:
    virtual void process_sync() override {
        core_assert(this->handler() != nullptr);
        this->handler()->read(*this);
    }

    virtual void register_processing_request() override {
        core_assert(this->handler() != nullptr);
        this->handler()->set_read_request(*this, [this]() { this->request_processing(); });
    }

public:
    virtual std::span<const T> data() const override {
        return std::span(static_cast<const T *>(this->raw()->bptr), this->raw()->nord);
    }
    virtual std::span<T> data() override {
        return std::span(static_cast<T *>(this->raw()->bptr), this->raw()->nord);
    }

    virtual size_t max_length() const override {
        return this->raw()->nelm;
    }

    [[nodiscard]] virtual bool set_data(std::span<const T> new_data) override {
        if (new_data.size() > max_length()) {
            return false;
        }
        std::copy_n(new_data.data(), new_data.size(), data().data());
        this->raw()->nord = new_data.size();
        return true;
    }
};

template <typename T, typename R>
class EpicsOutputArrayRecord : public virtual OutputArrayRecord<T>, public EpicsArrayRecordBase<T, R, OutputArrayHandler<T>> {
public:
    using EpicsArrayRecordBase<T, R, OutputArrayHandler<T>>::EpicsArrayRecordBase;

protected:
    virtual void process_sync() override {
        core_assert(this->handler() != nullptr);
        this->handler()->write(*this);
    }

    virtual void register_processing_request() override {
        core_unimplemented();
    }

public:
    virtual std::span<const T> data() const override {
        return std::span(static_cast<const T *>(this->raw()->bptr), this->raw()->nord);
    }

    virtual size_t max_length() const override {
        return this->raw()->nelm;
    }
};
