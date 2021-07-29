#pragma once

#include <cstdint>
#include <utility>

#include <core/assert.hpp>
#include <record/array.hpp>

#include <waveformRecord.h>

#include "base.hpp"
#include "types.hpp"

template <typename T>
class WaveformRecord final :
    public virtual InputArrayRecord<T>,
    public EpicsRecord
{
public:
    explicit WaveformRecord(waveformRecord *raw) : EpicsRecord((dbCommon *)raw) {
        assert_eq(epics_type_enum<T>, data_type());
    }

    const waveformRecord *raw() const {
        return (const waveformRecord *)EpicsRecord::raw();
    }
    waveformRecord *raw() {
        return (waveformRecord *)EpicsRecord::raw();
    }

private:
    menuFtype data_type() const {
        return static_cast<menuFtype>(raw()->ftvl);
    }

    const void *raw_data() const {
        return raw()->bptr;
    }
    void *raw_data() {
        return raw()->bptr;
    }

protected:
    const InputArrayHandler<T> *handler() const {
        return static_cast<const InputArrayHandler<T> *>(EpicsRecord::handler());
    }
    InputArrayHandler<T> *handler() {
        return static_cast<InputArrayHandler<T> *>(EpicsRecord::handler());
    }

    virtual void process_sync() override {
        std::cout << "process_sync: " << (size_t)handler() << std::endl;
        if (handler() != nullptr) {
            handler()->read(*this);
        }
    }

public:
    virtual const T *data() const override {
        return (const T *)raw_data();
    }

    virtual size_t max_length() const override {
        return raw()->nelm;
    }
    virtual size_t length() const override {
        return raw()->nord;
    }

    virtual void set_handler(std::unique_ptr<InputArrayHandler<T>> &&handler) override {
        EpicsRecord::set_handler(std::move(handler));
    }
};
