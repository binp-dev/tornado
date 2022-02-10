#pragma once

#include <cstdint>
#include <utility>

#include <dbAccess.h>

#include <core/assert.hpp>
#include <record/value.hpp>

#include "base.hpp"

template <typename Raw>
class EpicsValueRecordBase : public EpicsRecord
{
public:
    explicit EpicsValueRecordBase(Raw *raw_) : EpicsRecord((dbCommon *)raw_) {}

    const Raw *raw() const {
        return (const Raw *)EpicsRecord::raw();
    }
    Raw *raw() {
        return (Raw *)EpicsRecord::raw();
    }
};

template <typename T, typename Raw>
class EpicsInputValueRecord :
    public virtual InputValueRecord<T>,
    public EpicsValueRecordBase<Raw>
{
public:
    explicit EpicsInputValueRecord(Raw *raw) : EpicsValueRecordBase<Raw>(raw) {}

protected:
    const InputValueHandler<T> *handler() const {
        return static_cast<const InputValueHandler<T> *>(EpicsRecord::handler());
    }
    InputValueHandler<T> *handler() {
        return static_cast<InputValueHandler<T> *>(EpicsRecord::handler());
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
    virtual void set_handler(std::unique_ptr<InputValueHandler<T>> &&handler) override {
        EpicsRecord::set_handler(std::move(handler));
    }
};

template <typename T, typename Raw>
class EpicsOutputValueRecord :
    public virtual OutputValueRecord<T>,
    public EpicsValueRecordBase<Raw>
{
public:
    explicit EpicsOutputValueRecord(Raw *raw) : EpicsValueRecordBase<Raw>(raw) {}

protected:
    const OutputValueHandler<T> *handler() const {
        return static_cast<const OutputValueHandler<T> *>(EpicsRecord::handler());
    }
    OutputValueHandler<T> *handler() {
        return static_cast<OutputValueHandler<T> *>(EpicsRecord::handler());
    }

    virtual void process_sync() override {
        if (handler() != nullptr) {
            handler()->write(*this);
        }
    }

public:
    virtual void set_handler(std::unique_ptr<OutputValueHandler<T>> &&handler) override {
        EpicsRecord::set_handler(std::move(handler));
    }
};
