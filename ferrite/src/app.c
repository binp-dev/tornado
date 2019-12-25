#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

void panic() {
    taskDISABLE_INTERRUPTS();
    vTaskSuspendAll();
    while (1);
}
