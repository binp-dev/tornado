#pragma once

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//! @brief IPP message types from App to MCU
typedef enum {
    IPP_APP_NONE               = 0x00, /* none */
    IPP_APP_START              = 0x01, /* start signal */
    IPP_APP_STOP               = 0x02, /* stop all operation */
    IPP_APP_DAC_SET            = 0x11, /* set DAC value */
    IPP_APP_ADC_REQ            = 0x12, /* request ADC value */
} IppTypeApp;

//! @brief IPP message types from MCU to App
typedef enum {
    IPP_MCU_NONE               = 0x00, /* none */
    IPP_MCU_ADC_VAL            = 0x10, /* ADC value */
    IPP_MCU_ERROR              = 0xE0, /* error report */
    IPP_MCU_DEBUG              = 0xE1  /* debug message */
} IppTypeMcu;


//! @brief IPP load message status
typedef enum {
    IPP_LOAD_OK                 = 0x00, /* ok */
    IPP_LOAD_OUT_OF_BOUNDS      = 0x01, /* max buffer length is too short */
    IPP_LOAD_PARSE_ERROR        = 0x02, /* message parse error */
} IppLoadStatus;

/* Concrete message types */

/* App -> MCU */

typedef struct {
    uint32_t value;
} _IppMsgAppDacSet;

#define _IPP_MSG_APP_DAC_SET_LEN 3
IppLoadStatus _ipp_msg_app_load_dac_set(_IppMsgAppDacSet *dst, const uint8_t *src, size_t max_length);
void _ipp_msg_app_store_dac_set(const _IppMsgAppDacSet *src, uint8_t *dst);

typedef struct {
    uint8_t index;
} _IppMsgAppAdcReq;

#define _IPP_MSG_APP_ADC_REQ_LEN 1
IppLoadStatus _ipp_msg_app_load_adc_req(_IppMsgAppAdcReq *dst, const uint8_t *src, size_t max_length);
void _ipp_msg_app_store_adc_req(const _IppMsgAppAdcReq *src, uint8_t *dst);

/* MCU -> App */

typedef struct {
    uint8_t index;
    uint32_t value;
} _IppMsgMcuAdcVal;

#define _IPP_MSG_MCU_ADC_VAL_LEN (1 + 3)
IppLoadStatus _ipp_msg_mcu_load_adc_val(_IppMsgMcuAdcVal *dst, const uint8_t *src, size_t max_length);
void _ipp_msg_mcu_store_adc_val(const _IppMsgMcuAdcVal *src, uint8_t *dst);

typedef struct {
    //! TODO: Use enum for error codes.
    uint8_t code;
    const char *message;
} _IppMsgMcuError;

size_t _ipp_msg_mcu_len_error(const _IppMsgMcuError *msg);
IppLoadStatus _ipp_msg_mcu_load_error(_IppMsgMcuError *dst, const uint8_t *src, size_t max_length);
void _ipp_msg_mcu_store_error(const _IppMsgMcuError *src, uint8_t *dst);

typedef struct {
    const char *message;
} _IppMsgMcuDebug;

size_t _ipp_msg_mcu_len_debug(const _IppMsgMcuDebug *msg);
IppLoadStatus _ipp_msg_mcu_load_debug(_IppMsgMcuDebug *dst, const uint8_t *src, size_t max_length);
void _ipp_msg_mcu_store_debug(const _IppMsgMcuDebug *src, uint8_t *dst);


//! @brief IPP base message from App to MCU.
typedef struct {
    IppTypeApp type;
    union {
        _IppMsgAppDacSet dac_set;
        _IppMsgAppAdcReq adc_req;
    };
} IppMsgAppAny;

//! @brief Length of stored App message.
size_t ipp_msg_app_len(const IppMsgAppAny *msg);

//! @brief Load App message from bytes.
IppLoadStatus ipp_msg_app_load(IppMsgAppAny *dst, const uint8_t *src, size_t max_length);

//! @brief Store App message to bytes.
void ipp_msg_app_store(const IppMsgAppAny *src, uint8_t *dst);

//! @brief IPP base message from MCU to App.
typedef struct {
    IppTypeMcu type;
    union {
        _IppMsgMcuAdcVal adc_val;
        _IppMsgMcuError error;
        _IppMsgMcuDebug debug;
    };
} IppMsgMcuAny;

//! @brief Length of stored MCU message.
size_t ipp_msg_mcu_len(const IppMsgMcuAny *msg);

//! @brief Load MCU message from bytes.
IppLoadStatus ipp_msg_mcu_load(IppMsgMcuAny *dst, const uint8_t *src, size_t max_length);

//! @brief Store MCU message to bytes.
void ipp_msg_mcu_store(const IppMsgMcuAny *src, uint8_t *dst);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
