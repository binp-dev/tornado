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

IppLoadStatus _ipp_msg_app_load_wf_data(_IppMsgAppWfData *dst, const uint8_t *src, size_t max_length) {
    if (max_length < 2) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    size_t len = load_size16(src);
    if (len + 2 > max_length) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    dst->data = src + 2;
    dst->len = len;
    return IPP_LOAD_OK;
}

void _ipp_msg_app_store_wf_data(const _IppMsgAppWfData *src, uint8_t *dst) {
    store_size16(dst, src->len);
    if (src->data != dst + 2) {
        memcpy(dst + 2, src->data, src->len);
    }
}


size_t _ipp_msg_mcu_len_error(const _IppMsgMcuError *msg) {
    return 1 + strlen(msg->message) + 1;
}

IppLoadStatus _ipp_msg_mcu_load_error(_IppMsgMcuError *dst, const uint8_t *src, size_t max_length) {
    if (max_length < 2) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    if (strnlen((const char *)(src + 1), max_length - 1) == (max_length - 1)) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    dst->code = src[0];
    dst->message = (const char *)(src + 1);
    return IPP_LOAD_OK;
}

void _ipp_msg_mcu_store_error(const _IppMsgMcuError *src, uint8_t *dst) {
    dst[0] = src->code;
    if (dst + 1 != (const uint8_t *)src->message) {
        strcpy((char *)(dst + 1), src->message);
    }
}


size_t _ipp_msg_mcu_len_debug(const _IppMsgMcuDebug *msg) {
    return strlen(msg->message) + 1;
}

IppLoadStatus _ipp_msg_mcu_load_debug(_IppMsgMcuDebug *dst, const uint8_t *src, size_t max_length) {
    if (max_length < 1) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    if (strnlen((const char *)src, max_length) == max_length) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    dst->message = (const char *)src;
    return IPP_LOAD_OK;
}

void _ipp_msg_mcu_store_debug(const _IppMsgMcuDebug *src, uint8_t *dst) {
    if (dst != (const uint8_t *)src->message) {
        strcpy((char *)dst, src->message);
    }
}


size_t ipp_msg_app_len(const IppMsgAppAny *msg) {
    size_t len = 0;
    switch (msg->type) {
    case IPP_APP_NONE: break;
    case IPP_APP_START: break;
    case IPP_APP_STOP: break;
    case IPP_APP_WF_DATA: len = _ipp_msg_app_len_wf_data(&msg->wf_data); break;
    }
    return 1 + len;
}

IppLoadStatus ipp_msg_app_load(IppMsgAppAny *dst, const uint8_t *src, size_t max_length) {
    if (max_length < 1) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    dst->type = (IppTypeApp)src[0];
    switch (src[0]) {
    case (uint8_t)IPP_APP_NONE: return IPP_LOAD_OK;
    case (uint8_t)IPP_APP_START: return IPP_LOAD_OK;
    case (uint8_t)IPP_APP_STOP: return IPP_LOAD_OK;
    case (uint8_t)IPP_APP_WF_DATA: return _ipp_msg_app_load_wf_data(&dst->wf_data, src + 1, max_length - 1);
    default: return IPP_LOAD_PARSE_ERROR;
    }
}

void ipp_msg_app_store(const IppMsgAppAny *src, uint8_t *dst) {
    switch (src->type) {
    case IPP_APP_NONE: break;
    case IPP_APP_START: break;
    case IPP_APP_STOP: break;
    case IPP_APP_WF_DATA: _ipp_msg_app_store_wf_data(&src->wf_data, dst + 1); break;
    }
    dst[0] = (uint8_t)src->type;
}

size_t ipp_msg_mcu_len(const IppMsgMcuAny *msg) {
    size_t len = 0;
    switch (msg->type) {
    case IPP_MCU_NONE: break;
    case IPP_MCU_WF_REQ: break;
    case IPP_MCU_ERROR: len = _ipp_msg_mcu_len_error(&msg->error); break;
    case IPP_MCU_DEBUG: len = _ipp_msg_mcu_len_debug(&msg->debug); break;
    }
    return 1 + len;
}

IppLoadStatus ipp_msg_mcu_load(IppMsgMcuAny *dst, const uint8_t *src, size_t max_length) {
    if (max_length < 1) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    dst->type = (IppTypeMcu)src[0];
    switch (src[0]) {
    case (uint8_t)IPP_MCU_NONE: return IPP_LOAD_OK;
    case (uint8_t)IPP_MCU_WF_REQ: return IPP_LOAD_OK;
    case (uint8_t)IPP_MCU_ERROR: return _ipp_msg_mcu_load_error(&dst->error, src + 1, max_length - 1);
    case (uint8_t)IPP_MCU_DEBUG: return _ipp_msg_mcu_load_debug(&dst->debug, src + 1, max_length - 1);
    default: return IPP_LOAD_PARSE_ERROR; 
    }
}

void ipp_msg_mcu_store(const IppMsgMcuAny *src, uint8_t *dst) {
    switch (src->type) {
    case IPP_MCU_NONE: break;
    case IPP_MCU_WF_REQ: break;
    case IPP_MCU_ERROR: _ipp_msg_mcu_store_error(&src->error, dst + 1); break;
    case IPP_MCU_DEBUG: _ipp_msg_mcu_store_debug(&src->debug, dst + 1); break;
    }
    dst[0] = (uint8_t)src->type;
}
