#pragma once

#include <fsl_gpio.h>

//! FIXME: Remove this constraint.
#define HAL_GPIO_MAX_GROUP_COUNT 8

#define _HAL_GPIO_BLOCK_START 1
#define _HAL_GPIO_BLOCK_END 6
#define _HAL_GPIO_BLOCK_COUNT (_HAL_GPIO_BLOCK_END - _HAL_GPIO_BLOCK_START)
#define _HAL_GPIO_BASE_COUNT _HAL_GPIO_BLOCK_COUNT

//! BlockIndex starts from _HAL_GPIO_BLOCK_START, BaseIndex starts from 0
typedef HalGpioBlockIndex _HalGpioBaseIndex;

/*!
 * @brief GPIO group handle.
 * @note Should have fixed address, cannot be moved.
 */
typedef struct {
    volatile uint32_t intrs[_HAL_GPIO_BASE_COUNT];
    volatile HalGpioIntrCallback callback;
    void *volatile user_data;
} HalGpioGroup;


/*! @brief GPIO specific pin handle. */
typedef struct {
    HalGpioGroup *group;
    GPIO_Type *base;
    _HalGpioBaseIndex base_index; // starts from 0
    HalGpioPinIndex index;
    enum _gpio_pin_direction direction;
    enum _gpio_interrupt_mode intr_mode;
} HalGpioPin;
