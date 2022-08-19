#include "_interface.h"

#include <aaiRecord.h>
#include <aaoRecord.h>
#include <aiRecord.h>
#include <aoRecord.h>
#include <biRecord.h>
#include <boRecord.h>
#include <mbbiDirectRecord.h>
#include <mbboDirectRecord.h>
#include <waveformRecord.h>

#include "_array_record.h"
#include "_record.h"

void fer_var_request_proc(FerVar *var) {
    fer_epics_record_request_proc((dbCommon *)var);
}

void fer_var_proc_done(FerVar *var) {
    fer_epics_record_proc_done((dbCommon *)var);
}

const char *fer_var_name(FerVar *var) {
    return ((dbCommon *)var)->name;
}

FerVarType fer_var_type(FerVar *var) {
    return fer_epics_record_var_info((dbCommon *)var)->type;
}

void *fer_var_data(FerVar *var) {
    return fer_epics_record_var_info((dbCommon *)var)->data;
}

size_t fer_var_array_len(FerVar *var) {
    (size_t)(*fer_epics_record_var_array_info((dbCommon *)var)->len_ptr);
}

void fer_var_array_set_len(FerVar *var, size_t new_size) {
    assert(new_size <= fer_var_type(var).array_max_len);
    *fer_epics_record_var_array_info((dbCommon *)var)->len_ptr = (epicsUInt32)new_size;
}

void *fer_var_user_data(FerVar *var) {
    return fer_epics_record_dpvt((dbCommon *)var)->user_data;
}

void fer_var_set_user_data(FerVar *var, void *user_data) {
    fer_epics_record_dpvt((dbCommon *)var)->user_data = user_data;
}
