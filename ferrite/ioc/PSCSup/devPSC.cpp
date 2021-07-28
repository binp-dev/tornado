#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExit.h>
#include <epicsExport.h>

#include "framework.hpp"

void init(void) {
    std::cout << "*** PSC Device Support ***" << std::endl;
    framework_init();
}

epicsExportRegistrar(init);
