#include "app_debug.h"

#include <ipp.h>

#ifdef APP_DEBUG_IO_RPMSG
volatile uint8_t __app_io_rpmsg_enabled = 0;
uint8_t __app_io_rpmsg_buffer[0x100] = {0};

void APP_Debug_IO_RPMSG_Enable() {
    __app_io_rpmsg_enabled = 1;
}

void APP_Debug_IO_RPMSG_Send(int len) {
    if (__app_io_rpmsg_enabled) {
        __app_io_rpmsg_buffer[0] = IPP_MCU_DEBUG;
        __app_io_rpmsg_buffer[0xFF] = 0;
        APP_RPMSG_Send(__app_io_rpmsg_buffer, 0x100);
    }
}
#endif // APP_DEBUG_IO_RPMSG
