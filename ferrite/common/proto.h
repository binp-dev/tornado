#pragma once

//! IPP message types from App to MCU
#define IPP_APP_NONE           0x00 /* none */
#define IPP_APP_START          0x01 /* start signal */
#define IPP_APP_STOP           0x02 /* stop all operation */
#define IPP_APP_DAC_SET        0x11 /* set DAC value */
#define IPP_APP_ADC_REQ        0x12 /* request ADC value */

//! IPP message types from MCU to App
#define IPP_MCU_NONE           0x00 /* none */
#define IPP_MCU_ADC_VAL        0x10 /* ADC value */
#define IPP_MCU_ERROR          0xE0 /* error report */
#define IPP_MCU_DEBUG          0xE1 /* debug message */
