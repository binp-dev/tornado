#include "ipp.h"
#include <string.h>


static uint32_t load_uint24(const uint8_t *ptr) {
    return (uint32_t)ptr[0] | (uint32_t)ptr[1] << 8 | (uint32_t)ptr[2] << 16;
}
static uint8_t store_uint24(uint8_t *ptr, const uint32_t val) {
    if (val >= (uint32_t)1 << 24) {
        return 1;
    }
    ptr[0] = (uint8_t)(val & 0xFF);
    ptr[1] = (uint8_t)((val >> 8) & 0xFF);
    ptr[2] = (uint8_t)((val >> 16) & 0xFF);
    return 0;
}

IppLoadStatus _ipp_msg_app_load_dac_set(_IppMsgAppDacSet *dst, const uint8_t *src, size_t max_length) {
    if (max_length < _IPP_MSG_APP_DAC_SET_LEN) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    dst->value = load_uint24(src);
    return IPP_LOAD_OK;
}

void _ipp_msg_app_store_dac_set(const _IppMsgAppDacSet *src, uint8_t *dst) {
    store_uint24(dst, src->value);
}

IppLoadStatus _ipp_msg_app_load_adc_req(_IppMsgAppAdcReq *dst, const uint8_t *src, size_t max_length) {
    if (max_length < _IPP_MSG_APP_ADC_REQ_LEN) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    dst->index = src[0];
    return IPP_LOAD_OK;
}

void _ipp_msg_app_store_adc_req(const _IppMsgAppAdcReq *src, uint8_t *dst) {
    dst[0] = src->index;
}


IppLoadStatus _ipp_msg_mcu_load_adc_val(_IppMsgMcuAdcVal *dst, const uint8_t *src, size_t max_length) {
    if (max_length < _IPP_MSG_MCU_ADC_VAL_LEN) {
        return IPP_LOAD_OUT_OF_BOUNDS;
    }
    dst->index = src[0];
    dst->value = load_uint24(src + 1);
    return IPP_LOAD_OK;
}

void _ipp_msg_mcu_store_adc_val(const _IppMsgMcuAdcVal *src, uint8_t *dst) {
    dst[0] = src->index;
    store_uint24(dst + 1, src->value);
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
    case IPP_APP_DAC_SET: len = _IPP_MSG_APP_DAC_SET_LEN; break;
    case IPP_APP_ADC_REQ: len = _IPP_MSG_APP_ADC_REQ_LEN; break;
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
    case (uint8_t)IPP_APP_DAC_SET: return _ipp_msg_app_load_dac_set(&dst->dac_set, src + 1, max_length - 1);
    case (uint8_t)IPP_APP_ADC_REQ: return _ipp_msg_app_load_adc_req(&dst->adc_req, src + 1, max_length - 1);
    default: return IPP_LOAD_PARSE_ERROR;
    }
}

void ipp_msg_app_store(const IppMsgAppAny *src, uint8_t *dst) {
    switch (src->type) {
    case IPP_APP_NONE: break;
    case IPP_APP_START: break;
    case IPP_APP_STOP: break;
    case IPP_APP_DAC_SET: _ipp_msg_app_store_dac_set(&src->dac_set, dst + 1); break;
    case IPP_APP_ADC_REQ: _ipp_msg_app_store_adc_req(&src->adc_req, dst + 1); break;
    }
    dst[0] = (uint8_t)src->type;
}

size_t ipp_msg_mcu_len(const IppMsgMcuAny *msg) {
    size_t len = 0;
    switch (msg->type) {
    case IPP_MCU_NONE: break;
    case IPP_MCU_ADC_VAL: len = _IPP_MSG_MCU_ADC_VAL_LEN; break;
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
    case (uint8_t)IPP_MCU_ADC_VAL: return _ipp_msg_mcu_load_adc_val(&dst->adc_val, src + 1, max_length - 1);
    case (uint8_t)IPP_MCU_ERROR: return _ipp_msg_mcu_load_error(&dst->error, src + 1, max_length - 1);
    case (uint8_t)IPP_MCU_DEBUG: return _ipp_msg_mcu_load_debug(&dst->debug, src + 1, max_length - 1);
    default: return IPP_LOAD_PARSE_ERROR; 
    }
}

void ipp_msg_mcu_store(const IppMsgMcuAny *src, uint8_t *dst) {
    switch (src->type) {
    case IPP_MCU_NONE: break;
    case IPP_MCU_ADC_VAL: _ipp_msg_mcu_store_adc_val(&src->adc_val, dst + 1); break;
    case IPP_MCU_ERROR: _ipp_msg_mcu_store_error(&src->error, dst + 1); break;
    case IPP_MCU_DEBUG: _ipp_msg_mcu_store_debug(&src->debug, dst + 1); break;
    }
    dst[0] = (uint8_t)src->type;
}
