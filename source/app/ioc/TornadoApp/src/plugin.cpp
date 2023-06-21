#include <iostream>
#include <cmath>

#include <epicsThread.h>

extern "C" void app_set_dac_corr(double value);

extern "C" void app_plugin_main(void *) {
    std::cout << "Plugin started" << std::endl;

    double time = 0.0;
    const double step = 0.001;

    const double mag = 1.0;
    const double freq = 50.0;
    for (;;) {
        const double value = mag * (long(time / step) % 5 == 0);
        // const double value = mag * sin(2 * M_PI * freq * time);
        std::cout << "Set DAC corr: " << value << std::endl;
        app_set_dac_corr(value);

        epicsThreadSleep(step);
        time += step;
    }
}
