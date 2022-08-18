#pragma once

#include <stdlib.h>

/// Opaque variable.
typedef struct FerAppVar FerAppVar;

/// Direction of the variable.
typedef enum FerVarDir {
    FER_VAR_DIR_READ = 0,
    FER_VAR_DIR_WRITE,
} FerVarDir;

/// Kind of variable.
typedef enum FerVarKind {
    FER_VAR_KIND_SCALAR = 0,
    FER_VAR_KIND_ARRAY,
} FerVarKind;

/// Scalar value type.
typedef enum FerVarScalarType {
    /// Variable does not contain scalars.
    FER_VAR_SCALAR_TYPE_NONE = 0,
    FER_VAR_SCALAR_TYPE_U8,
    FER_VAR_SCALAR_TYPE_I8,
    FER_VAR_SCALAR_TYPE_U16,
    FER_VAR_SCALAR_TYPE_I16,
    FER_VAR_SCALAR_TYPE_U32,
    FER_VAR_SCALAR_TYPE_I32,
    FER_VAR_SCALAR_TYPE_U64,
    FER_VAR_SCALAR_TYPE_I64,
    FER_VAR_SCALAR_TYPE_F32,
    FER_VAR_SCALAR_TYPE_F64,
} FerVarScalarType;

/// Initialize application.
extern void fer_app_init();
/// All initialization complete, safe to start operating.
extern void fer_app_start();

/// Initialize variable.
extern void fer_var_init(FerAppVar *var);
/// Request record processing.
void fer_var_proc_request(FerAppVar *var);
/// Asynchronous variable processing start.
extern void fer_var_proc_start(FerAppVar *var);
/// Notify that asynchronous variable processing complete.
void fer_var_proc_done(FerAppVar *var);

/// Variable name.
const char *fer_var_name(FerAppVar *var);

/// Direction of the variable.
FerVarDir fer_var_dir(FerAppVar *var);
/// Kind of the variable.
FerVarKind fer_var_kind(FerAppVar *var);
/// Type of scalars in the variable if it contains scalars.
FerVarScalarType fer_var_scal_type(FerAppVar *var);
/// Maximum number of items in the variable if it is array.
size_t fer_var_array_max_size(FerAppVar *var);

/// Raw variable data that must be interpreted according to variable type.
void *fer_var_data(FerAppVar *var);
/// Current number of items in variable.
/// Always less or equal than `fer_var_array_max_size`.
/// Variable must be an array.
size_t fer_var_array_size(FerAppVar *var);
/// Set new number of items in variable.
/// Must be less or equal than `fer_var_array_max_size`.
/// Variable must be an array.
size_t fer_var_array_set_size(FerAppVar *var, size_t new_size);
