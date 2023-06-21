#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsExit.h"
#include "epicsThread.h"
#include "iocsh.h"

extern "C" void app_plugin_main();

static void thread_main(void *) {
    app_plugin_main();
}

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        iocsh(argv[1]);
        epicsThreadSleep(.2);
    }

    epicsThreadCreate("plugin", epicsThreadPriorityMedium, epicsThreadStackMedium, thread_main, nullptr);

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
