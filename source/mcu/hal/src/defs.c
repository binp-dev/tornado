#include <hal/defs.h>

const char *hal_retcode_str(hal_retcode code) {
    switch (code) {
    case HAL_SUCCESS:
        return "SUCCESS";
    case HAL_FAILURE:
        return "FAILURE";
    case HAL_BAD_ALLOC:
        return "BAD_ALLOC";
    case HAL_OUT_OF_BOUNDS:
        return "OUT_OF_BOUNDS";
    case HAL_INVALID_INPUT:
        return "INVALID_INPUT";
    case HAL_INVALID_DATA:
        return "INVALID_DATA";
    case HAL_UNIMPLEMENTED:
        return "UNIMPLEMENTED";
    case HAL_TIMED_OUT:
        return "TIMED_OUT";
    default:
        return "Unknown code";
    }
}
