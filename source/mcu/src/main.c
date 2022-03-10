#include <stdint.h>
#include <stdbool.h>

#include <fsl_common.h
#include <fsl_gpio.h

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

#include <common/config.h>
#include <ipp.h>

#include <drivers/skifio.h>
#include <utils/ringbuf.h>

#include <tasks/config.h>
#ifdef GENERATE_SYNC
#include <tasks/sync.h>
#endif
#include <tasks/stats.h>


int main(void) {
    /* Initialize standard SDK demo application pins */
    /* M7 has its local cache and enabled by default,
     * need to set smart subsystems (0x28000000 ~ 0x3FFFFFFF)
     * non-cacheable before accessing this address region */
    BOARD_InitMemory();

    /* Board specific RDC settings */
    BOARD_RdcInit();

    BOARD_BootClockRUN();

    hal_io_uart_init(3);

    copyResourceTable();

#ifdef MCMGR_USED
    /* Initialize MCMGR before calling its API */
    (void)MCMGR_Init();
#endif

    hal_print("\n\r\n\r");
    hal_log_info("** Board started **");

    hal_log_info("Create statistics task");
    xTaskCreate(task_stats, "Statistics task", TASK_STACK_SIZE, NULL, STATS_TASK_PRIORITY, NULL);

    hal_log_info("Create RPMsg tasks");
    xTaskCreate(task_rpmsg_send, "RPMsg send task", TASK_STACK_SIZE, NULL, RPMSG_SEND_TASK_PRIORITY, NULL);
    xTaskCreate(task_rpmsg_recv, "RPMsg receive task", TASK_STACK_SIZE, NULL, RPMSG_RECV_TASK_PRIORITY, NULL);

#ifdef GENERATE_SYNC
    hal_log_info("Create sync generator task");
    xTaskCreate(sync_generator_task, "Sync generator task", TASK_STACK_SIZE, NULL, SYNC_TASK_PRIORITY, NULL);
#endif

    vTaskStartScheduler();
    // Must never return.
    hal_panic();
}
