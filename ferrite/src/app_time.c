#include "app_time.h"


TickType_t APP_Time_Ms2Ticks(uint32_t ms) {
    if (ms == 0) {
        return 0;
    } else if (ms == APP_FOREVER_MS) {
        return portMAX_DELAY;
    } else {
        return (ms - 1)/portTICK_PERIOD_MS + 1;
    }
}
