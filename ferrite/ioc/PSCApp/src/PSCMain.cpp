#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsExit.h"
#include "epicsThread.h"
#include "iocsh.h"

#define INTERACTIVE

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        iocsh(argv[1]);
        epicsThreadSleep(.2);
    }
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
