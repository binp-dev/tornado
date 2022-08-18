#pragma once

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include <dbCommon.h>
#include <dbScan.h>

#include "_interface.h"

/// Private data to store in a record.
typedef struct FerEpicsRecordData {
    /// Scan list for I/O Intr.
    /// NULL if record scanning is not an `I/O Intr`.
    IOSCANPVT ioscan_list;
} FerEpicsRecordData;

/// Initialize record.
void fer_epics_record_init(dbCommon *rec) {
    FerEpicsRecordData *dpvt = (FerEpicsRecordData *)malloc(sizeof(FerEpicsRecordData));
    assert(dpvt != NULL);
    dpvt->ioscan_list = NULL;

    assert(rec->dpvt == NULL);
    rec->dpvt = dpvt;

    fer_app_var_init((FerAppVar *)rec);
}

/// Deinitialize record.
void fer_epics_record_deinit(dbCommon *rec) {
    if (rec->dpvt != NULL) {
        free((void *)rec->dpvt);
        rec->dpvt = NULL;
    }
}

/// Get private data from record.
FerEpicsRecordData *fer_epics_record_dpvt(dbCommon *rec) {
    FerEpicsRecordData *dpvt = (FerEpicsRecordData *)rec->dpvt;
    assert(dpvt != NULL);
    return dpvt;
}

/// Initialize record scan list.
IOSCANPVT fer_epics_record_ioscan_create(dbCommon *rec) {
    FerEpicsRecordData *dpvt = fer_epics_record_dpvt(rec);

    IOSCANPVT ioscan_list;
    scanIoInit(&ioscan_list);
    dpvt->ioscan_list = ioscan_list;
    return ioscan_list;
}

/// Process record.
void fer_epics_record_process(dbCommon *rec) {
    if (!rec->pact) {
        dbScanLock(rec);
        rec->pact = true;
        fer_var_proc_start((FerAppVar *)rec);
    } else {
        rec->pact = false;
        dbScanUnlock(rec);
    }
}

void fer_epics_record_proc_done(dbCommon *rec) {
    /// Should we call this from callback?
    struct typed_rset *rset = (struct typed_rset *)rec->rset;
    rset->process(rec);
}
