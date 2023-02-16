#include <FreeRTOS.h>

#include <hal/assert.h>
#include <hal/io.h>

#include "device/board.h"
#include "device/clock_config.h"
#include "device/rsc_table.h"

#include <drivers/sync.h>

#ifdef GENERATE_SYNC
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
    hal_io_uart_init(3);

    copyResourceTable();

#ifdef MCMGR_USED
    // Initialize MCMGR before calling its API
    (void)MCMGR_Init();
#endif

    hal_print("\n\r\n\r");
    hal_log_info("** Board started **");

    user_main();

#ifdef GENERATE_SYNC
    SyncGenerator sync;
    sync_generator_init(&sync);
    sync_generator_start(&sync, SYNC_PERIOD_US);
    hal_log_info("Sync generator started");
#endif

    vTaskStartScheduler();
    // Must never return.
    hal_panic();
}
