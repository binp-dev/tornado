#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsExit.h"
#include "epicsThread.h"
#include "iocsh.h"

extern "C" void app_plugin_main(void *);

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        iocsh(argv[1]);
        epicsThreadSleep(.2);
    }

    epicsThreadCreate(
        "plugin",
        epicsThreadPriorityHigh,
        epicsThreadStackMedium,
        app_plugin_main,
        nullptr //
    );

#ifdef INTERACTIVE
    iocsh(NULL);
#else
    // Sleep forever
    for (;;) {
        epicsThreadSleep(1.0);
    }
#endif
    epicsExit(0);
    return 0;
}
