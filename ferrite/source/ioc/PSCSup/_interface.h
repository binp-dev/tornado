#pragma once

#include <stddef.h>

/// Opaque variable.
typedef struct FerVar FerVar;

/// Kind of variable.
typedef enum FerVarKind {
    FER_VAR_KIND_SCALAR = 0,
    FER_VAR_KIND_ARRAY,
} FerVarKind;

/// Direction of the variable.
typedef enum FerVarDir {
    FER_VAR_DIR_READ = 0,
    FER_VAR_DIR_WRITE,
} FerVarDir;

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

/// Variable type.
typedef struct FerVarType {
    /// Kind of the variable.
    FerVarKind kind;
    /// Direction of the variable.
    FerVarDir dir;
    /// Type of scalars in the variable if it contains scalars.
    FerVarScalarType scalar_type;
    /// Maximum number of items in the variable if it is array.
    size_t array_max_len;
} FerVarType;

/// Initialize application.
extern void fer_app_init();
/// All initialization complete, safe to start operating.
extern void fer_app_start();

/// Initialize variable.
extern void fer_var_init(FerVar *var);
/// Request record processing.
void fer_var_request_proc(FerVar *var);
/// Asynchronous variable processing start.
extern void fer_var_proc_start(FerVar *var);
/// Notify that asynchronous variable processing complete.
void fer_var_proc_done(FerVar *var);

/// Lock variable.
/// Following operations require variable to be locked.
void fer_var_lock(FerVar *var);
/// Unlock variable.
void fer_var_unlock(FerVar *var);

/// Variable name.
const char *fer_var_name(FerVar *var);
/// Variable type information.
FerVarType fer_var_type(FerVar *var);

/// Raw variable data that must be interpreted according to variable type.
void *fer_var_data(FerVar *var);
/// Current number of items in variable.
/// Always less or equal than `fer_var_array_max_size`.
/// Variable must be an array.
size_t fer_var_array_len(FerVar *var);
/// Set new number of items in variable.
/// Must be less or equal than `fer_var_array_max_size`.
/// Variable must be an array.
void fer_var_array_set_len(FerVar *var, size_t new_size);

/// Get user data.
void *fer_var_user_data(FerVar *var);
/// Set user data.
void fer_var_set_user_data(FerVar *var, void *user_data);
