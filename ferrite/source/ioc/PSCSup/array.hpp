#pragma once

#include <cstdint>
#include <cstring>
#include <utility>

#include <dbAccess.h>
#include <epicsTypes.h>

#include <core/assert.hpp>
#include <record/array.hpp>

#include "base.hpp"
#include "types.hpp"

template <typename T, typename Raw>
class EpicsArrayRecordBase : public EpicsRecord
{
public:
    explicit EpicsArrayRecordBase(Raw *raw_) : EpicsRecord((dbCommon *)raw_) {
        assert_eq(epics_type_enum<T>, data_type());
        assert_eq(dbValueSize(data_type()), sizeof(T));
    }

    const Raw *raw() const {
        return (const Raw *)EpicsRecord::raw();
    }
    Raw *raw() {
        return (Raw *)EpicsRecord::raw();
    }

protected:
    menuFtype data_type() const {
        return static_cast<menuFtype>(raw()->ftvl);
    }
    const void *raw_data() const {
        return raw()->bptr;
    }
    void *raw_data() {
        return raw()->bptr;
    }
};

template <typename T, typename Raw>
class EpicsInputArrayRecord :
    public virtual InputArrayRecord<T>,
    public EpicsArrayRecordBase<T, Raw>
{
public:
    explicit EpicsInputArrayRecord(Raw *raw) : EpicsArrayRecordBase<T, Raw>(raw) {}

protected:
    const InputArrayHandler<T> *handler() const {
        return static_cast<const InputArrayHandler<T> *>(EpicsRecord::handler());
    }
    InputArrayHandler<T> *handler() {
        return static_cast<InputArrayHandler<T> *>(EpicsRecord::handler());
    }

    virtual void process_sync() override {
        if (handler() != nullptr) {
            handler()->read(*this);
        }
    }

    virtual void register_processing_request() override {
        if (handler() != nullptr) {
            handler()->set_read_request(*this, [this]() {
                this->request_processing();
            });
        }
    }

public:
    virtual void set_handler(std::unique_ptr<InputArrayHandler<T>> &&handler) override {
        EpicsRecord::set_handler(std::move(handler));
    }

public:
    virtual const T *data() const override {
        return (const T *)this->raw_data();
    }
    virtual T *data() override {
        return (T *)this->raw_data();
    }

    virtual size_t max_length() const override {
        return this->raw()->nelm;
    }
    virtual size_t length() const override {
        return this->raw()->nord;
    }
    [[nodiscard]]
    virtual bool set_length(size_t length) override {
        if (length <= max_length()) {
            this->raw()->nord = length;
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] virtual bool set_data(const T *new_data, size_t length) override {
        if (!set_length(length)) {
            return false;
        }
        std::copy(new_data, new_data + length, data());
        return true;
    }
};

template <typename T, typename Raw>
class EpicsOutputArrayRecord :
    public virtual OutputArrayRecord<T>,
    public EpicsArrayRecordBase<T, Raw>
{
public:
    explicit EpicsOutputArrayRecord(Raw *raw) : EpicsArrayRecordBase<T, Raw>(raw) {}

protected:
    const OutputArrayHandler<T> *handler() const {
        return static_cast<const OutputArrayHandler<T> *>(EpicsRecord::handler());
    }
    OutputArrayHandler<T> *handler() {
        return static_cast<OutputArrayHandler<T> *>(EpicsRecord::handler());
    }

    virtual void process_sync() override {
        if (handler() != nullptr) {
            handler()->write(*this);
        }
    }

public:
    virtual void set_handler(std::unique_ptr<OutputArrayHandler<T>> &&handler) override {
        EpicsRecord::set_handler(std::move(handler));
    }

public:
    virtual const T *data() const override {
        return (const T *)this->raw_data();
    }

    virtual size_t max_length() const override {
        return this->raw()->nelm;
    }
    virtual size_t length() const override {
        return this->raw()->nord;
    }
};