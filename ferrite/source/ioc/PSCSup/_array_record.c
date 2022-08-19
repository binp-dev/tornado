#include "_array.h"

FerEpicsVarArray *fer_epics_record_var_array_info(dbCommon *rec) {
    FerEpicsVar *var_info = fer_epics_record_var_info(rec);
    assert(var_info->type.kind == FER_VAR_KIND_ARRAY);
    return (FerEpicsVarArray *)var_info;
}

FerVarScalarType fer_epics_convert_scalar_type(menuFtype ftype) {
    switch (ftype) {
    case menuFtypeCHAR:
        return FER_VAR_SCALAR_TYPE_I8;
    case menuFtypeUCHAR:
        return FER_VAR_SCALAR_TYPE_U8;
    case menuFtypeSHORT:
        return FER_VAR_SCALAR_TYPE_I16;
    case menuFtypeUSHORT:
        return FER_VAR_SCALAR_TYPE_U16;
    case menuFtypeLONG:
        return FER_VAR_SCALAR_TYPE_I32;
    case menuFtypeULONG:
        return FER_VAR_SCALAR_TYPE_U32;
    case menuFtypeINT64:
        return FER_VAR_SCALAR_TYPE_I64;
    case menuFtypeUINT64:
        return FER_VAR_SCALAR_TYPE_U64;
    case menuFtypeFLOAT:
        return FER_VAR_SCALAR_TYPE_F32;
    case menuFtypeDOUBLE:
        return FER_VAR_SCALAR_TYPE_F64;
    default:
        return FER_VAR_SCALAR_TYPE_NONE;
    }
}
