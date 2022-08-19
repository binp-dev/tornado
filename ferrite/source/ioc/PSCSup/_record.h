#pragma once

#include <callback.h>
#include <dbCommon.h>
#include <dbScan.h>
#include <menuFtype.h>

#include "_interface.h"

typedef enum FerEpicsRecordType {
    FER_EPICS_RECORD_TYPE_AI,
    FER_EPICS_RECORD_TYPE_AO,
    FER_EPICS_RECORD_TYPE_BI,
    FER_EPICS_RECORD_TYPE_BO,
    FER_EPICS_RECORD_TYPE_WAVEFORM,
    FER_EPICS_RECORD_TYPE_AAI,
    FER_EPICS_RECORD_TYPE_AAO,
    FER_EPICS_RECORD_TYPE_MBBI_DIRECT,
    FER_EPICS_RECORD_TYPE_MBBO_DIRECT,
} FerEpicsRecordType;

typedef struct FerEpicsVar {
    FerVarType type;
    void *data;
} FerEpicsVar;

/// Private data to store in a record.
typedef struct FerEpicsRecordDpvt {
    /// Type of the record.
    FerEpicsRecordType type;
    /// Callback to call when async processing is done.
    epicsCallback process;
    /// Scan list for I/O Intr.
    /// NULL if record scanning is not an `I/O Intr`.
    IOSCANPVT ioscan_list;
    /// Interface variable informaion.
    FerEpicsVar *var_info;
    /// User data.
    void *user_data;
} FerEpicsRecordDpvt;

/// Initialize record.
void fer_epics_record_init(dbCommon *rec, FerEpicsRecordType type, FerEpicsVar *var_info);
/// Deinitialize record.
void fer_epics_record_deinit(dbCommon *rec);

/// Get private data from record.
FerEpicsRecordDpvt *fer_epics_record_dpvt(dbCommon *rec);
/// Get interface variable info.
FerEpicsVar *fer_epics_record_var_info(dbCommon *rec);

/// Initialize record scan list.
IOSCANPVT fer_epics_record_ioscan_create(dbCommon *rec);

/// Request record processing
void fer_epics_recore_request_proc(dbCommon *rec);
/// Process record.
void fer_epics_record_process(dbCommon *rec);
/// Notify that process is done.
void fer_epics_record_proc_done(dbCommon *rec);
