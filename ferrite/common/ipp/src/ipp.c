#include "ipp.h"
#include <string.h>


static size_t load_size16(const uint8_t *ptr) {
    return (size_t)ptr[0] | (size_t)ptr[1] << 8;
}
static uint8_t store_size16(uint8_t *ptr, const size_t val) {
    if (val >= (size_t)1 << 16) {
        return 1;
    }
    ptr[1] = (uint8_t)(val & 0xFF);
    ptr[2] = (uint8_t)((val >> 8) & 0xFF);
    return 0;
}


size_t _ipp_app_len_wf_data(const IppAppWfData *msg) {
    return 1 + 2 + msg->len;
}

IppAppWfData _ipp_app_load_wf_data(const uint8_t *src) {
    IppAppWfData msg = {
        .data = &src[3],
        .len = load_size16(&src[1])
    };
    return msg;
}

void _ipp_app_store_wf_data(const IppAppWfData *src, uint8_t *dst) {
    store_size16(&dst[1], src->len);
    if (src->data != &dst[3]) {
        memcpy(dst[3], src->data, src->len);
    }
}


size_t _ipp_mcu_len_error(const IppMcuError *msg) {
    return 1 + 1 + strlen(msg->message);
}

IppMcuError _ipp_mcu_load_error(const uint8_t *src) {
    IppMcuError msg = {
        .code = src[1],
        .message = &src[2]
    };
    return msg;
}

void _ipp_mcu_store_error(const IppMcuError *src, uint8_t *dst) {
    dst[1] = src->code;
    if (&dst[2] != src->message) {
        strcpy(&dst[2], src->message);
    }
}


size_t _ipp_mcu_len_debug(const IppMcuDebug *msg) {
    return 1 + 2 + strlen(msg->message);
}

IppMcuDebug _ipp_mcu_load_debug(const uint8_t *src) {
    IppMcuDebug msg = {
        .message = &src[2]
    };
    return msg;
}

void _ipp_mcu_store_debug(const IppMcuDebug *src, uint8_t *dst) {
    if (&dst[2] != src->message) {
        strcpy(&dst[2], src->message);
    }
}


size_t ipp_app_len(const IppAppMessage *msg) {
    switch (msg->type) {
    case IPP_APP_NONE: return 1;
    case IPP_APP_START: return 1;
    case IPP_APP_STOP: return 1;
    case IPP_APP_WF_DATA: return _ipp_app_len_wf_data(msg);
    }
}

IppAppMessage ipp_app_load(const uint8_t *src) {
    IppAppMessage dst = { .type = (IppAppType)src[0] };
    switch (dst.type) {
    case IPP_APP_NONE: break;
    case IPP_APP_START: break;
    case IPP_APP_STOP: break;
    case IPP_APP_WF_DATA: dst.wf_data = _ipp_app_load_wf_data(src);
    }
    return dst;
}

void ipp_app_store(const IppAppMessage *src, uint8_t *dst) {
    switch (src->type) {
    case IPP_APP_NONE: break;
    case IPP_APP_START: break;
    case IPP_APP_STOP: break;
    case IPP_APP_WF_DATA: _ipp_app_store_wf_data(&src->wf_data, dst);
    }
    dst[0] = (uint8_t)src->type;
}

size_t ipp_mcu_len(const IppMcuMessage *msg) {
    switch (msg->type) {
    case IPP_MCU_NONE: return 1;
    case IPP_MCU_WF_REQ: return 1;
    case IPP_MCU_ERROR: return _ipp_mcu_len_error(msg);
    case IPP_MCU_DEBUG: return _ipp_mcu_len_debug(msg);
    }
}

IppMcuMessage ipp_mcu_load(const uint8_t *src) {
    IppMcuMessage dst = { .type = (IppMcuType)src[0] };
    switch (dst.type) {
    case IPP_MCU_NONE: break;
    case IPP_MCU_WF_REQ: break;
    case IPP_MCU_ERROR: dst.error = _ipp_mcu_load_error(src);
    case IPP_MCU_DEBUG: dst.debug = _ipp_mcu_load_debug(src);
    }
    return dst;
}

void ipp_mcu_store(const IppMcuMessage *src, uint8_t *dst) {
    switch (src->type) {
    case IPP_MCU_NONE: break;
    case IPP_MCU_WF_REQ: break;
    case IPP_MCU_ERROR: _ipp_mcu_store_error(&src->error, dst);
    case IPP_MCU_DEBUG: _ipp_mcu_store_debug(&src->debug, dst);
    }
    dst[0] = (uint8_t)src->type;
}
