#include <iostream>
#include <cmath>

#include <epicsTime.h>
#include <epicsThread.h>

extern "C" void app_set_dac_corr(double value);

extern "C" void app_plugin_main(void *) {
    std::cout << "Plugin started" << std::endl;

    double time = 0.0;
    const double step = 0.001;

    const double mag = 1.0;
    const double freq = 50.0;
    for (;;) {
        epicsTimeStamp ts_begin;
        epicsTimeGetCurrent(&ts_begin);

        const double value = mag * (long(time / step) % 5 == 0);
        // const double value = mag * sin(2 * M_PI * freq * time);
        std::cout << "Set DAC corr: " << value << std::endl;
        app_set_dac_corr(value);

        epicsTimeStamp ts_end;
        epicsTimeGetCurrent(&ts_end);
        const double diff = epicsTimeDiffInSeconds(&ts_end, &ts_begin);
        if (diff < 0.0) {
            epicsThreadSleep(step);
        } else if (diff < step) {
            epicsThreadSleep(step - diff);
        }
        time += step;
    }
}
