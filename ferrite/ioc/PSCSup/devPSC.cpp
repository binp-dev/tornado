#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExit.h>
#include <epicsExport.h>

#include <core/panic.hpp>
#include <framework.hpp>

// FIXME: Figure out why this function isn't called anymore.
void init(void) {
    std::cout << "*** PSC Device Support ***" << std::endl;
    set_panic_hook([]() {
        epicsExit(1);
    });
    framework_init();
}

epicsExportRegistrar(init);
