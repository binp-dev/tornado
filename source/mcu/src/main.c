#include <stdint.h>
#include <stdbool.h>

#include <fsl_common.h>
#include <fsl_gpio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <hal/assert.h>
#include <hal/io.h>
#include <hal/rpmsg.h>
#include <hal/math.h>
#include <hal/time.h>

#include "device/board.h"
#include "device/clock_config.h"
#include "device/rsc_table.h"

#include <tasks/stats.h>
#include <tasks/control.h>
#include <tasks/rpmsg.h>
#ifdef GENERATE_SYNC
#include <tasks/sync.h>
#endif


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

    Statistics stats;
    stats_reset(&stats);

    Control control;
    control_init(&control, &stats);
    Rpmsg rpmsg;
    rpmsg_init(&rpmsg, &control, &stats);

    hal_log_info("Enable statistics report");
    stats_report_run(&stats);

    hal_log_info("Start SkifIO control process");
    control_run(&control);

    hal_log_info("Start RPMSG communication");
    rpmsg_run(&rpmsg);

#ifdef GENERATE_SYNC
    hal_log_info("Start sync generator");
    sync_generator_run(&stats);
#endif

    vTaskStartScheduler();
    // Must never return.
    hal_panic();
}
