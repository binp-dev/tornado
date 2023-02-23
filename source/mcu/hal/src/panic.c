#include <hal/panic.h>

#include "FreeRTOS.h"
#include "task.h"

uint8_t __ustd_panicked = 0;

void __ustd_panic() {
    __ustd_panicked = 1;
    taskDISABLE_INTERRUPTS();
    vTaskSuspendAll();
    while (1) {}
}
