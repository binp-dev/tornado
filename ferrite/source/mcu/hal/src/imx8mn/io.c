#include <hal/io.h>

#ifdef HAL_PRINT_UART

#include <stdbool.h>

#include "fsl_debug_console.h"
#include "fsl_iomuxc.h"
#include "pin_mux.h"

static void init_mux(
    uint32_t ta,
    uint32_t tb,
    uint32_t tc,
    uint32_t td,
    uint32_t te,

    uint32_t ra,
    uint32_t rb,
    uint32_t rc,
    uint32_t rd,
    uint32_t re //
) {
    IOMUXC_SetPinMux(ra, rb, rc, rd, re, 0U);
    IOMUXC_SetPinConfig(ra, rb, rc, rd, re, IOMUXC_SW_PAD_CTL_PAD_DSE(6U) | IOMUXC_SW_PAD_CTL_PAD_FSEL(2U));
    IOMUXC_SetPinMux(ta, tb, tc, td, te, 0U);
    IOMUXC_SetPinConfig(ta, tb, tc, td, te, IOMUXC_SW_PAD_CTL_PAD_DSE(6U) | IOMUXC_SW_PAD_CTL_PAD_FSEL(2U));
}

void hal_io_uart_init(uint32_t instance) {
    bool warn = false;
    clock_root_control_t clock_root;
    clock_ip_name_t clock;
    switch (instance) {
    case 3:
        init_mux(IOMUXC_UART3_TXD_UART3_TX, IOMUXC_UART3_RXD_UART3_RX);
        clock_root = kCLOCK_RootUart3;
        clock = kCLOCK_Uart3;
        break;

    default:
        instance = 4;
        warn = true;
    case 4:
        init_mux(IOMUXC_UART4_TXD_UART4_TX, IOMUXC_UART4_RXD_UART4_RX);
        clock_root = kCLOCK_RootUart4;
        clock = kCLOCK_Uart4;
        break;
    }
    uint32_t baudrate = 115200u;
    uint32_t uartClkSrcFreq = CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / CLOCK_GetRootPreDivider(clock_root)
        / CLOCK_GetRootPostDivider(clock_root) / 10;
    CLOCK_EnableClock(clock);
    DbgConsole_Init(instance, baudrate, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);

    if (warn) {
        PRINTF("[WARN] IO UART: Unsupported instance, falling back to %ld.", instance);
    }
}

#endif
