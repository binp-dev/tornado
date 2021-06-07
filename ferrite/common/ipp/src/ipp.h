#pragma once

#include <stdlib.h>
#include <stdint.h>


//! @brief IPP message types from App to MCU
typedef enum {
    IPP_APP_NONE               = 0x00, /* none */
    IPP_APP_START              = 0x01, /* start signal */
    IPP_APP_STOP               = 0x02, /* stop all operation */
    IPP_APP_WF_DATA            = 0x11, /* waveform data */
} IppAppType;

//! @brief IPP message types from MCU to App
typedef enum {
    IPP_MCU_NONE               = 0x00, /* none */
    IPP_MCU_WF_REQ             = 0x10, /* waveform request */
    IPP_MCU_ERROR              = 0xE0, /* error report */
    IPP_MCU_DEBUG              = 0xE1  /* debug message */
} IppMcuType;


/* Concrete message types */

/* App -> MCU */

typedef struct {
    const uint8_t *data;
    size_t len;
} IppAppWfData;

size_t _ipp_app_len_wf_data(const IppAppWfData *msg);
IppAppWfData _ipp_app_load_wf_data(const uint8_t *src);
void _ipp_app_store_wf_data(const IppAppWfData *src, uint8_t *dst);

/* MCU -> App */

typedef struct {
    //! TODO: Use enum for error codes.
    uint8_t code;
    const char *message;
} IppMcuError;

size_t _ipp_mcu_len_error(const IppMcuError *msg);
IppMcuError _ipp_mcu_load_error(const uint8_t *src);
void _ipp_mcu_store_error(const IppMcuError *src, uint8_t *dst);

typedef struct {
    const char *message;
} IppMcuDebug;

size_t _ipp_mcu_len_debug(const IppMcuDebug *msg);
IppMcuDebug _ipp_mcu_load_debug(const uint8_t *src);
void _ipp_mcu_store_debug(const IppMcuDebug *src, uint8_t *dst);


//! @brief IPP base message from App to MCU.
typedef struct {
    IppAppType type;
    union {
        IppAppWfData wf_data;
    };
} IppAppMessage;

//! @brief Length of stored App message.
size_t ipp_app_len(const IppAppMessage *msg);

//! @brief Load App message from bytes.
void ipp_app_load(IppAppMessage *dst, const uint8_t *src);

//! @brief Store App message to bytes.
void ipp_app_store(const IppAppMessage *src, uint8_t *dst);

/* Custom constructors */

IppAppMessage ipp_app_new_none();
IppAppMessage ipp_app_new_start();
IppAppMessage ipp_app_new_stop();
IppAppMessage ipp_app_new_wf_data(const uint8_t *data, size_t len);


//! @brief IPP base message from MCU to App.
typedef struct {
    IppMcuType type;
    union {
        IppMcuError error;
        IppMcuDebug debug;
    };
} IppMcuMessage;

//! @brief Length of stored MCU message.
size_t ipp_mcu_len(const IppMcuMessage *msg);

//! @brief Load MCU message from bytes.
void ipp_mcu_load(IppMcuMessage *dst, const uint8_t *src);

//! @brief Store MCU message to bytes.
void ipp_mcu_store(const IppMcuMessage *src, uint8_t *dst);

/* Custom constructors */

IppMcuMessage ipp_mcu_new_none();
IppMcuMessage ipp_mcu_new_wf_req();
IppMcuMessage ipp_mcu_new_error(uint8_t code, const char *message);
IppMcuMessage ipp_mcu_new_debug(const char *message);
