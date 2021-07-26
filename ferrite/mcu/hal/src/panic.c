#include <hal/panic.h>

#include "FreeRTOS.h"
#include "task.h"

bool __hal_panicked = false;

void __hal_panic() {
    taskDISABLE_INTERRUPTS();
    vTaskSuspendAll();
    while (1) {}
}
