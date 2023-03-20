#include <hal/atomic.h>

#include <FreeRTOS.h>
#include <task.h>

#define _HAL_ATOMIC_DEF_BASE(name, type) \
    type hal_atomic_##name##_load(const hal_atomic_##name##_t *self) { \
        taskENTER_CRITICAL(); \
        type value = self->value; \
        taskEXIT_CRITICAL(); \
        return value; \
    } \
\
    void hal_atomic_##name##_store(hal_atomic_##name##_t *self, type new_value) { \
        taskENTER_CRITICAL(); \
        self->value = new_value; \
        taskEXIT_CRITICAL(); \
    } \
\
    void hal_atomic_##name##_add(hal_atomic_##name##_t *self, type arg) { \
        taskENTER_CRITICAL(); \
        self->value += arg; \
        taskEXIT_CRITICAL(); \
    }

#define _HAL_ATOMIC_DEF_UNSIGNED(name, type) \
    _HAL_ATOMIC_DEF_BASE(name, type) \
\
    type hal_atomic_##name##_sub_checked(hal_atomic_##name##_t *self, type arg) { \
        type ov = 0; \
        taskENTER_CRITICAL(); \
        type val = self->value; \
        if (val >= arg) { \
            self->value = val - arg; \
        } else { \
            self->value = 0; \
            ov = arg - val; \
        } \
        taskEXIT_CRITICAL(); \
        return ov; \
    }

_HAL_ATOMIC_DEF_UNSIGNED(size, size_t)
