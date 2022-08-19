#include "stdlib.h"

#include <alarm.h>
#include <dbDefs.h>
#include <devSup.h>
#include <epicsExit.h>
#include <epicsExport.h>
#include <initHooks.h>
#include <recGbl.h>
#include <registryFunction.h>

#include "_interface.h"

static void init_hooks(initHookState state) {
    switch (state) {
    case initHookAfterIocRunning:
        fer_app_start();
        break;
    default:
        break;
    }
}

void fer_epics_app_init(void) {
    initHookRegister(init_hooks);
    fer_app_init();
}

epicsExportRegistrar(fer_epics_app_init);
