#include <FreeRTOS.h>
#include <task.h>

#include <hal/assert.h>
#include <hal/io.h>

#include "device/board.h"
#include "device/clock_config.h"
#include "device/rsc_table.h"

#include <tasks/stats.h>
#include <tasks/control.h>
#include <tasks/rpmsg.h>
#include <tasks/sync.h>


// The stack of `main` is tiny, so we store our state as globals.
#ifdef GENERATE_SYNC
#define SYNC_PERIOD_US 100
SyncGenerator sync;
#endif
Statistics stats;
Control control;
Rpmsg rpmsg;


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

    stats_reset(&stats);
#ifdef GENERATE_SYNC
    sync_generator_init(&sync, SYNC_PERIOD_US, &stats);
#endif
    control_init(&control, &stats);
    rpmsg_init(&rpmsg, &control, &stats);

    hal_log_info("Enable statistics report");
    stats_report_run(&stats);

    hal_log_info("Start SkifIO control process");
    control_run(&control);

    hal_log_info("Start RPMSG communication");
    rpmsg_run(&rpmsg);

#ifdef GENERATE_SYNC
    hal_log_info("Start sync generator");
    sync_generator_run(&sync);
#endif

    vTaskStartScheduler();
    // Must never return.
    hal_panic();
}
