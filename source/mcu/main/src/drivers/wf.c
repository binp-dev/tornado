#include "wf.h"

#include <fsl_common.h>
#include <device/board.h>

const size_t wf_max_offset = WFBUFFER_LEN;

const size_t wf_offset_align = __SCB_DCACHE_LINE_SIZE;

volatile uint8_t *wf_addr(size_t offset) {
    return (volatile uint8_t *)(WFBUFFER_BASE + offset);
}

void wf_acquire(uint8_t *addr, size_t len) {
    SCB_InvalidateDCache_by_Addr((uint32_t *)addr, len);
}
void wf_release(uint8_t *addr, size_t len) {
    SCB_CleanDCache_by_Addr((uint32_t *)addr, len);
}
