#include <cstdlib>
#include <iostream>
#include <atomic>

#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExit.h>
#include <epicsExport.h>
#include <dbDefs.h>
#include <registryFunction.h>
#include <initHooks.h>

#include <core/panic.hpp>
#include <core/log.hpp>
#include <framework.hpp>

extern "C" {
void psc_init(void);
}

static void init_hooks(initHookState state) {
    switch (state) {
    case initHookAfterIocRunning:
        framework_start();
        break;
    default:
        break;
    }
}

void psc_init(void) {
    set_panic_hook([]() { epicsExit(1); });

    initHookRegister(init_hooks);

    framework_init();
}

epicsExportRegistrar(psc_init);
