#pragma once

#include <epicsTypes.h>
#include <menuFtype.h>

#include "_interface.h"
#include "_record.h"

typedef struct FerEpicsVarArray {
    FerEpicsVar base;
    epicsUInt32 *len_ptr;
} FerEpicsVarArray;

FerEpicsVarArray *fer_epics_record_var_array_info(dbCommon *rec);

FerVarScalarType fer_epics_convert_scalar_type(menuFtype ftype);
