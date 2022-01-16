#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <hal/defs.h>

typedef uint8_t HalGpioBlockIndex;
typedef uint8_t HalGpioPinIndex;
typedef uint32_t HalGpioPinMask;

#if defined(HAL_IMX7)
#include "imx7/gpio.h"
#elif defined(HAL_IMX8MN)
#include "imx8mn/gpio.h"
#else
#error "Unknown target"
#endif

typedef enum {
    HAL_GPIO_INPUT = 0,
    HAL_GPIO_OUTPUT,
} HalGpioDirection;

typedef enum {
    HAL_GPIO_INTR_DISABLED = 0,
    HAL_GPIO_INTR_LOW_LEVEL,
    HAL_GPIO_INTR_HIGH_LEVEL,
    HAL_GPIO_INTR_RISING_EDGE,
    HAL_GPIO_INTR_FALLING_EDGE,
    HAL_GPIO_INTR_RISING_OR_FALLING_EDGE,
} HalGpioIntrMode;

/*!
 * @brief Initialize group.
 * @param[out] group Group handle to be written after successful call.
 * @return Return code.
 */
hal_retcode hal_gpio_group_init(HalGpioGroup *group);

/*!
 * @brief Deinitialize group.
 * @note All group pins must be deinitialized before.
 * @param[in] group Group handle to deinitialize.
 * @return Return code.
 */
hal_retcode hal_gpio_group_deinit(HalGpioGroup *group);

/*!
 * @brief Configure interrupt callback for all pins in group.
 * @param[in] callback Function pointer to be called. NULL means that interrupts are disabled.
 * @param[in] user_data Data to pass to interrupt callback.
 * @return Return code.
 */
hal_retcode hal_gpio_group_set_intr(
    HalGpioGroup *group,
    void (*callback)(void *, HalGpioBlockIndex, HalGpioPinMask),
    void *user_data);

/*!
 * @brief Initialize specific pin.
 * @param[out] pin Pin handle to be written after successful call.
 * @param[in] group Group handle to attach pin to.
 * @param[in] base_index GPIO block index.
 * @param[in] pin_index Index of specific pin in block.
 * @param[in] direction Input or output direction.
 * @param[in] intr_mode Interrupt mode for pin.
 * @return Return code.
 */
hal_retcode hal_gpio_pin_init(
    HalGpioPin *pin,
    HalGpioGroup *group,
    HalGpioBlockIndex block_index,
    HalGpioPinIndex pin_index,
    HalGpioDirection direction,
    HalGpioIntrMode intr_mode //
);

/*!
 * @brief Deinitialize specific pin.
 * @param[in] pin Pin handle.
 * @return Return code.
 */
void hal_gpio_pin_deinit(HalGpioPin *pin);

/*! @brief Read pin state. */
bool hal_gpio_pin_read(const HalGpioPin *pin);

/*! @brief Write pin state. */
void hal_gpio_pin_write(const HalGpioPin *pin, bool value);
