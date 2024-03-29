#include <hal/time.h>

#define ns_to_steps(ns) ((15 * (ns)-1) / 100 + 1)

uint32_t hal_busy_wait_ns(uint64_t ns) {
    if (ns == 0) {
        return 0;
    }
    const uint64_t steps = ns_to_steps(ns);
    uint32_t x = 0;
    for (uint64_t i = 0; i < steps; ++i) {
        x = 1103515245 * x + 12345;
    }
    return x;
}
