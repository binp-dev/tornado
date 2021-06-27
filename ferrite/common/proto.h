#pragma once

//! IPP message types from App to MCU
#define IPP_APP_NONE           0x00 /* none */
#define IPP_APP_START          0x01 /* start signal */
#define IPP_APP_STOP           0x02 /* stop all operation */
#define IPP_APP_WF_DATA        0x11 /* waveform data */

//! IPP message types from MCU to App
#define IPP_MCU_NONE           0x00 /* none */
#define IPP_MCU_WF_REQ         0x10 /* waveform request */
#define IPP_MCU_ERROR          0xE0 /* error report */
#define IPP_MCU_DEBUG          0xE1 /* debug message */
