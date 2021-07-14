#include <common/main.h>

int main(void) {
    BOARD_RdcInit();
    BOARD_ClockInit();

    return common_main();
}
