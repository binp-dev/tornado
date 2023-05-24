#include <FreeRTOS.h>
#include <task.h>

#include <hal/assert.h>
#include <hal/io.h>

#include "device/board.h"
#include "device/clock_config.h"
#include "device/rsc_table.h"


#ifdef GENERATE_SYNC
#include <drivers/sync.h>

#define SYNC_PERIOD_US 100
#endif

extern void user_main();

int main(void) {
    // M7 has its local cache and enabled by default, need to set smart subsystems (0x28000000 ~ 0x3FFFFFFF) non-cacheable
    // before accessing this address region
    BOARD_InitMemory();

    // Board specific RDC settings
    BOARD_RdcInit();

    BOARD_BootClockRUN();

    // Initialize UART I/O
    hal_io_init(3);

    copyResourceTable();

#ifdef MCMGR_USED
    // Initialize MCMGR before calling its API
    (void)MCMGR_Init();
#endif

    hal_print("\n\r\n\r");
    hal_print("** Board started **");

    user_main();

#ifdef GENERATE_SYNC
    sync_init();
    sync_start(SYNC_PERIOD_US);
    hal_print("Sync generator started");
#endif

    vTaskStartScheduler();
    // Must never return.
    hal_unreachable();
}
