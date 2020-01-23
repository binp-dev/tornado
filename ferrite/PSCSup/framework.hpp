#pragma once

#include <cstdio>
#include <string>
#include <type_traits>
#include <memory>

#include "record/record.hpp"
#include "record/waveform_record.hpp"

#include "device.hpp"

class PrintWaveform : public WaveformHandler {
public:
    PrintWaveform() = default;
    ~PrintWaveform() override = default;

    void read(WaveformRecord &record) override {
        printf("count: %zd\n", record.waveform_length());
        printf("data: [ ");
        for (int i = 0; i < int(record.waveform_length()); ++i) {
            printf("%lf, ", record.waveform_data<double>()[i]);
        }
        printf("]\n");
    }
};

std::unique_ptr<WaveformHandler> framework_record_init_waveform(WaveformRecord &record) {
    if (strcmp(record.name(), "WAVEFORM") == 0) {
        // FIXME: Call this only once
        return std::make_unique<PrintWaveform>();
    } else {
        assert(false);
    }
}

#ifdef UNITTEST
#include <catch/catch.hpp>

TEST_CASE( "Dummy test", "[dummy]" ) {
    REQUIRE(1 + 1 == 2);
}
#endif //UNITTEST
