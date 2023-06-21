#include <iostream>
#include <cmath>

#include <epicsThread.h>

extern "C" void app_set_dac_corr(double value);

extern "C" void app_plugin_main() {
    std::cout << "Plugin started" << std::endl;

    double time = 0.0;
    const double step = 0.1;

    const double mag = 0.1;
    const double freq = 0.001;
    for (;;) {
        const double value = sin(2 * M_PI * freq * time);
        std::cout << "Set DAC corr: " << value << std::endl;
        app_set_dac_corr(value);
        epicsThreadSleep(step);
        time += step;
    }
}
