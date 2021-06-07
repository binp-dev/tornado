#include "ipp.h"
#include <string.h>


static size_t load_size16(const uint8_t *ptr) {
    return (size_t)ptr[0] | (size_t)ptr[1] << 8;
}
static uint8_t store_size16(uint8_t *ptr, const size_t val) {
    if (val >= (size_t)1 << 16) {
        return 1;
    }
    ptr[0] = (uint8_t)(val & 0xFF);
    ptr[1] = (uint8_t)((val >> 8) & 0xFF);
    return 0;
}


size_t _ipp_msg_app_len_wf_data(const _IppMsgAppWfData *msg) {
    return 2 + msg->len;
}

_IppMsgAppWfData _ipp_msg_app_load_wf_data(const uint8_t *src) {
    _IppMsgAppWfData msg = {
        .data = src + 2,
        .len = load_size16(src)
    };
    return msg;
}

void _ipp_msg_app_store_wf_data(const _IppMsgAppWfData *src, uint8_t *dst) {
    store_size16(dst, src->len);
    if (src->data != dst + 2) {
        memcpy(dst + 2, src->data, src->len);
    }
}


size_t _ipp_msg_mcu_len_error(const _IppMsgMcuError *msg) {
    return 1 + strlen(msg->message);
}

_IppMsgMcuError _ipp_msg_mcu_load_error(const uint8_t *src) {
    _IppMsgMcuError msg = {
        .code = src[0],
        .message = src + 1
    };
    return msg;
}

void _ipp_msg_mcu_store_error(const _IppMsgMcuError *src, uint8_t *dst) {
    dst[0] = src->code;
    if (dst + 1 != src->message) {
        strcpy(dst + 1, src->message);
    }
}


size_t _ipp_msg_mcu_len_debug(const _IppMsgMcuDebug *msg) {
    return 2 + strlen(msg->message);
}

_IppMsgMcuDebug _ipp_msg_mcu_load_debug(const uint8_t *src) {
    _IppMsgMcuDebug msg = {
        .message = src
    };
    return msg;
}

void _ipp_msg_mcu_store_debug(const _IppMsgMcuDebug *src, uint8_t *dst) {
    if (dst != src->message) {
        strcpy(dst, src->message);
    }
}


size_t ipp_msg_app_len(const IppMsgAppAny *msg) {
    switch (msg->type) {
    case IPP_APP_NONE: return 1;
    case IPP_APP_START: return 1;
    case IPP_APP_STOP: return 1;
    case IPP_APP_WF_DATA: return 1 + _ipp_msg_app_len_wf_data(msg);
    }
}

IppMsgAppAny ipp_msg_app_load(const uint8_t *src) {
    IppMsgAppAny dst = { .type = (IppTypeApp)src[0] };
    switch (dst.type) {
    case IPP_APP_NONE: break;
    case IPP_APP_START: break;
    case IPP_APP_STOP: break;
    case IPP_APP_WF_DATA: dst.wf_data = _ipp_msg_app_load_wf_data(src + 1);
    }
    return dst;
}

void ipp_msg_app_store(const IppMsgAppAny *src, uint8_t *dst) {
    switch (src->type) {
    case IPP_APP_NONE: break;
    case IPP_APP_START: break;
    case IPP_APP_STOP: break;
    case IPP_APP_WF_DATA: _ipp_msg_app_store_wf_data(&src->wf_data, dst + 1);
    }
    dst[0] = (uint8_t)src->type;
}

size_t ipp_msg_mcu_len(const IppMsgMcuAny *msg) {
    switch (msg->type) {
    case IPP_MCU_NONE: return 1;
    case IPP_MCU_WF_REQ: return 1;
    case IPP_MCU_ERROR: return 1 + _ipp_msg_mcu_len_error(msg);
    case IPP_MCU_DEBUG: return 1 + _ipp_msg_mcu_len_debug(msg);
    }
}

IppMsgMcuAny ipp_msg_mcu_load(const uint8_t *src) {
    IppMsgMcuAny dst = { .type = (IppTypeMcu)src[0] };
    switch (dst.type) {
    case IPP_MCU_NONE: break;
    case IPP_MCU_WF_REQ: break;
    case IPP_MCU_ERROR: dst.error = _ipp_msg_mcu_load_error(src + 1);
    case IPP_MCU_DEBUG: dst.debug = _ipp_msg_mcu_load_debug(src + 1);
    }
    return dst;
}

void ipp_msg_mcu_store(const IppMsgMcuAny *src, uint8_t *dst) {
    switch (src->type) {
    case IPP_MCU_NONE: break;
    case IPP_MCU_WF_REQ: break;
    case IPP_MCU_ERROR: _ipp_msg_mcu_store_error(&src->error, dst + 1);
    case IPP_MCU_DEBUG: _ipp_msg_mcu_store_debug(&src->debug, dst + 1);
    }
    dst[0] = (uint8_t)src->type;
}
