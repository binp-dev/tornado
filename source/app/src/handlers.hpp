#pragma once

#include <record/value.hpp>
#include <record/array.hpp>

#include <common/config.h>

#include <device.hpp>


class DeviceHandler : public virtual Handler {
protected:
    Device &device_;

    inline explicit DeviceHandler(Device &device) : Handler(false) /* unused */, device_(device) {}
};

class DacHandler final : public DeviceHandler, public OutputValueHandler<point_t> {
public:
    DacHandler(Device &device) : Handler(false), DeviceHandler(device) {}

    virtual void write(OutputValueRecord<point_t> &record) override {
        /// FIXME: Remove conversion.
        double voltage = double(record.value() - DAC_CODE_SHIFT) * 1e-6 * DAC_STEP_UV;
        device_.write_dac(std::span(&voltage, 1));
    }
};

class AdcHandler final : public DeviceHandler, public InputValueHandler<point_t> {
private:
    uint8_t index_;

public:
    AdcHandler(Device &device, uint8_t index) : Handler(false), DeviceHandler(device), index_(index) {}

    virtual void read(InputValueRecord<point_t> &record) override {
        record.set_value(device_.read_adc_last_value(index_));
    }

    virtual void set_read_request(InputValueRecord<point_t> &, std::function<void()> &&) override {
        core_unimplemented();
    }
};

class DoutHandler final : public DeviceHandler, public OutputValueHandler<uint32_t> {
public:
    DoutHandler(Device &device) : Handler(false), DeviceHandler(device) {}

    virtual void write(OutputValueRecord<uint32_t> &record) override {
        device_.write_dout(record.value());
    }
};

class DinHandler final : public DeviceHandler, public InputValueHandler<uint32_t> {
public:
    DinHandler(Device &device) : Handler(false), DeviceHandler(device) {}

    virtual void read(InputValueRecord<uint32_t> &record) override {
        record.set_value(device_.read_din());
    }

    virtual void set_read_request(InputValueRecord<uint32_t> &, std::function<void()> &&callback) override {
        device_.set_din_callback(std::move(callback));
    }
};

class DacWfHandler final : public DeviceHandler, public OutputArrayHandler<double> {
public:
    DacWfHandler(Device &device, OutputArrayRecord<double> &record) : Handler(true), DeviceHandler(device) {
        device_.init_dac(record.max_length());
    }

    virtual void write(OutputArrayRecord<double> &record) override {
        device_.write_dac(record.data());
    }
};

class AdcWfHandler final : public DeviceHandler, public InputArrayHandler<double> {
private:
    uint8_t index_;

public:
    AdcWfHandler(Device &device, InputArrayRecord<double> &record, uint8_t index) :
        Handler(true),
        DeviceHandler(device),
        index_(index) {
        device_.init_adc(index_, record.max_length());
    }

    virtual void read(InputArrayRecord<double> &record) override {
        auto data = device_.read_adc(index_);
        core_assert(record.set_data(data));
    }

    virtual void set_read_request(InputArrayRecord<double> &, std::function<void()> &&callback) override {
        device_.set_adc_callback(index_, std::move(callback));
    }
};

class WfReqHandler final : public DeviceHandler, public InputValueHandler<bool> {
public:
    WfReqHandler(Device &device) : Handler(false), DeviceHandler(device) {}

    virtual void read(InputValueRecord<bool> &record) override {
        record.set_value(device_.dac_req_flag());
    }

    virtual void set_read_request(InputValueRecord<bool> &, std::function<void()> &&callback) override {
        device_.set_dac_req_callback(std::move(callback));
    }
};

class DacPlaybackModeHandler final : public DeviceHandler, public OutputValueHandler<bool> {
public:
    DacPlaybackModeHandler(Device &device) : Handler(false), DeviceHandler(device) {}

    virtual void write(OutputValueRecord<bool> &record) override {
        auto mode = record.value() ? //
            Device::DacPlaybackMode::Cyclic :
            Device::DacPlaybackMode::OneShot;

        device_.set_dac_playback_mode(mode);
    }
};

class DacOpStateHandler final : public DeviceHandler, public OutputValueHandler<bool> {
public:
    DacOpStateHandler(Device &device) : Handler(false), DeviceHandler(device) {}

    virtual void write(OutputValueRecord<bool> &record) override {
        auto state = record.value() ? //
            Device::DacOperationState::Running :
            Device::DacOperationState::Stopped;

        device_.set_dac_operation_state(state);
    }
};

class StatsResetHandler final : public DeviceHandler, public OutputValueHandler<bool> {
public:
    StatsResetHandler(Device &device) : Handler(false), DeviceHandler(device) {}

    virtual void write(OutputValueRecord<bool> &) override {
        device_.reset_statistics();
    }
};
