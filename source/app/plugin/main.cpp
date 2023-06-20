#include <iostream>

extern void app_set_dac_corr(double value);

extern "C" void app_plugin_main() {
    std::cout << "Plugin started" << std::endl;
    app_set_dac_corr(0.0);
}
