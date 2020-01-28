#pragma once

/* A7 to M4 message ids */
#define PSCA_NONE              0x00      /* none */
#define PSCA_WF_DATA           0x11      /* waveform data */
#define PSCA_HALT              0xFA      /* stop all operation */
#define PSCA_RESET             0xFF      /* soft reset */

/* M4 to A7 message ids */ 
#define PSCM_NONE              0x00      /* none */
#define PSCM_START             0x01      /* start signal */
#define PSCM_WF_REQ            0x10      /* waveform request */
#define PSCM_ERROR             0xF1      /* error report */
