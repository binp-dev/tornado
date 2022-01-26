#pragma once

#include <record/value.hpp>
#include <record/array.hpp>

#include <device.hpp>


class DeviceHandler {
protected:
    Device &device_;

    DeviceHandler(Device &device) : device_(device) {}
    virtual ~DeviceHandler() = default;
};

class DacHandler final : public DeviceHandler, public OutputValueHandler<int32_t> {
public:
    DacHandler(Device &device) : DeviceHandler(device) {}

    virtual void write(OutputValueRecord<int32_t> &record) override {
        device_.write_dac(record.value());
    }

    virtual bool is_async() const override {
        return false;
    }
};

class AdcHandler final : public DeviceHandler, public InputValueHandler<int32_t> {
private:
    uint8_t index_;

public:
    AdcHandler(Device &device, uint8_t index) : DeviceHandler(device), index_(index) {}

    virtual void read(InputValueRecord<int32_t> &record) override {
        record.set_value(device_.read_adc(index_));
    }

    virtual void set_read_request(InputValueRecord<int32_t> &, std::function<void()> &&callback) override {
        device_.set_adc_callback(index_, std::move(callback));
    }

    virtual bool is_async() const override {
        return false;
    }
};

class DoutHandler final : public DeviceHandler, public OutputValueHandler<uint32_t> {
public:
    DoutHandler(Device &device) : DeviceHandler(device) {}

    virtual void write(OutputValueRecord<uint32_t> &record) override {
        device_.write_dout(record.value());
    }

    virtual bool is_async() const override {
        return false;
    }
};

class DinHandler final : public DeviceHandler, public InputValueHandler<uint32_t> {
public:
    DinHandler(Device &device) : DeviceHandler(device) {}

    virtual void read(InputValueRecord<uint32_t> &record) override {
        record.set_value(device_.read_din());
    }

    virtual void set_read_request(InputValueRecord<uint32_t> &, std::function<void()> &&callback) override {
        device_.set_din_callback(std::move(callback));
    }

    virtual bool is_async() const override {
        return false;
    }
};

class ScanFreqHandler final : public DeviceHandler, public OutputValueHandler<int32_t> {
public:
    ScanFreqHandler(Device &device) : DeviceHandler(device) {}

    virtual void write(OutputValueRecord<int32_t> &record) override {
        const auto freq = std::clamp(record.value(), 1, 10);
        const auto period = std::chrono::milliseconds(1000 / freq);
        device_.set_adc_req_period(period);
    }

    virtual bool is_async() const override {
        return false;
    }
};
