#pragma once

#include <cstdint>
#include <utility>

#include <dbAccess.h>

#include <core/assert.hpp>
#include <record/value.hpp>

#include "base.hpp"

template <typename R, typename H>
class EpicsValueRecordBase : public EpicsRecord<R>, public virtual HandledRecord<H> {
public:
    using EpicsRecord<R>::EpicsRecord;
};

template <typename T, typename R>
class EpicsInputValueRecord : public EpicsValueRecordBase<R, InputValueHandler<T>>, public virtual InputValueRecord<T> {
public:
    using EpicsValueRecordBase<R, InputValueHandler<T>>::EpicsValueRecordBase;

protected:
    virtual void process_sync() override {
        core_assert(this->handler() != nullptr);
        this->handler()->read(*this);
    }

    virtual void register_processing_request() override {
        core_assert(this->handler() != nullptr);
        this->handler()->set_read_request(*this, [this]() { this->request_processing(); });
    }
};

template <typename T, typename R>
class EpicsOutputValueRecord : public EpicsValueRecordBase<R, OutputValueHandler<T>>, public virtual OutputValueRecord<T> {
public:
    using EpicsValueRecordBase<R, OutputValueHandler<T>>::EpicsValueRecordBase;

protected:
    virtual void process_sync() override {
        core_assert(this->handler() != nullptr);
        this->handler()->write(*this);
    }

    virtual void register_processing_request() override {
        core_unimplemented();
    }
};
