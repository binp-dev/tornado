#include "board.h"

#include <hal/gpio.h>
#include <hal/panic.h>
#include <hal/log.h>

#define HAL_GPIO_IRQ_PRIORITY 4

static GPIO_Type *const BASES[_HAL_GPIO_BASE_COUNT] = {GPIO1, GPIO2, GPIO3, GPIO4, GPIO5};
static const enum IRQn IRQNS[_HAL_GPIO_BASE_COUNT][2] = {
    {GPIO1_Combined_0_15_IRQn, GPIO1_Combined_16_31_IRQn},
    {GPIO2_Combined_0_15_IRQn, GPIO2_Combined_16_31_IRQn},
    {GPIO3_Combined_0_15_IRQn, GPIO3_Combined_16_31_IRQn},
    {GPIO4_Combined_0_15_IRQn, GPIO4_Combined_16_31_IRQn},
    {GPIO5_Combined_0_15_IRQn, GPIO5_Combined_16_31_IRQn},
};
static volatile bool INTRS_STATE[_HAL_GPIO_BASE_COUNT][2] = {{false}};

static HalGpioGroup *volatile GROUPS[HAL_GPIO_MAX_GROUP_COUNT] = {NULL};
static volatile size_t GROUPS_COUNT = 0;


static volatile HalGpioPinMask *group_intrs_ptr(HalGpioGroup *group, _HalGpioBaseIndex base_index) {
    return &group->intrs[base_index];
}

HalGpioPinMask half_mask(bool upper) {
    return upper ? 0x7fff0000 : 0x0000ffff;
}

bool which_half(HalGpioPinIndex pin_index) {
    return pin_index >= 16;
}

static volatile HalGpioPinMask group_intrs_half(HalGpioGroup *group, _HalGpioBaseIndex base_index, bool upper) {
    return *group_intrs_ptr(group, base_index) & half_mask(upper);
}

static void update_irq(_HalGpioBaseIndex base_index, bool upper) {
    bool state = INTRS_STATE[base_index][upper];
    enum IRQn irqn = IRQNS[base_index][upper];
    if (state) {
        NVIC_SetPriority(irqn, HAL_GPIO_IRQ_PRIORITY);
        NVIC_EnableIRQ(irqn);
    } else {
        NVIC_DisableIRQ(irqn);
    }
}

static void set_intr_state(_HalGpioBaseIndex base_index, bool upper, bool new_state) {
    volatile bool *state = &INTRS_STATE[base_index][upper];
    if (*state != new_state) {
        *state = new_state;
        update_irq(base_index, upper);
    }
}

static void update_block_intr_state(_HalGpioBaseIndex base_index, bool upper) {
    bool intrs_exist = false;
    for (size_t i = 0; i < GROUPS_COUNT; ++i) {
        HalGpioGroup *group = GROUPS[i];
        intrs_exist |= (group_intrs_half(group, base_index, upper) != 0);
    }
    set_intr_state(base_index, upper, intrs_exist);
}

void pin_enable_intr(HalGpioPin *pin) {
    if (pin->intr_mode != kGPIO_NoIntmode) {
        HalGpioPinMask pin_mask = (HalGpioPinMask)1 << pin->index;

        *group_intrs_ptr(pin->group, pin->base_index) |= pin_mask;
        update_block_intr_state(pin->base_index, which_half(pin->index));

        GPIO_ClearPinsInterruptFlags(pin->base, pin_mask);
        GPIO_EnableInterrupts(pin->base, pin_mask);
    }
}

void pin_disable_intr(HalGpioPin *pin) {
    if (pin->intr_mode != kGPIO_NoIntmode) {
        HalGpioPinMask pin_mask = (HalGpioPinMask)1 << pin->index;
        GPIO_DisableInterrupts(pin->base, pin_mask);

        *group_intrs_ptr(pin->group, pin->base_index) &= ~pin_mask;
        update_block_intr_state(pin->base_index, which_half(pin->index));
    }
}


hal_retcode hal_gpio_group_init(HalGpioGroup *group) {
    for (size_t i = 0; i < _HAL_GPIO_BASE_COUNT; ++i) {
        group->intrs[i] = 0;
    }
    group->callback = NULL;
    group->user_data = NULL;

    if (GROUPS_COUNT + 1 >= HAL_GPIO_MAX_GROUP_COUNT) {
        return HAL_OUT_OF_BOUNDS;
    }
    GROUPS[GROUPS_COUNT] = group;
    GROUPS_COUNT += 1;

    return HAL_SUCCESS;
}

hal_retcode hal_gpio_group_deinit(HalGpioGroup *group) {
    size_t group_index = GROUPS_COUNT;
    for (size_t i = 0; i < GROUPS_COUNT; ++i) {
        if (group == GROUPS[i]) {
            group_index = i;
        }
    }
    if (group_index >= GROUPS_COUNT) {
        // Group is not registered
        return HAL_OUT_OF_BOUNDS;
    }
    for (size_t i = group_index; i < GROUPS_COUNT - 1; ++i) {
        GROUPS[i] = GROUPS[i + 1];
    }
    GROUPS_COUNT -= 1;

    return HAL_SUCCESS;
}

hal_retcode hal_gpio_group_set_intr(
    HalGpioGroup *group,
    void (*callback)(void *, HalGpioBlockIndex, HalGpioPinMask),
    void *user_data //
) {
    group->user_data = user_data;
    group->callback = callback;

    return HAL_SUCCESS;
}

hal_retcode hal_gpio_pin_init(
    HalGpioPin *pin,
    HalGpioGroup *group,
    HalGpioBlockIndex block_index,
    HalGpioPinIndex pin_index,
    HalGpioDirection direction,
    HalGpioIntrMode intr_mode //
) {
    bool registered = false;
    for (size_t i = 0; i < GROUPS_COUNT; ++i) {
        if (group == GROUPS[i]) {
            registered = true;
        }
    }
    if (!registered) {
        // Group is not registered.
        return HAL_INVALID_DATA;
    }
    pin->group = group;

    if (block_index < _HAL_GPIO_BLOCK_START || block_index >= _HAL_GPIO_BLOCK_END) {
        return HAL_OUT_OF_BOUNDS;
    }
    _HalGpioBaseIndex base_index = block_index - _HAL_GPIO_BLOCK_START;
    pin->base_index = base_index;
    GPIO_Type *base = BASES[base_index];
    pin->base = base;

    if (pin_index > 31) {
        return HAL_OUT_OF_BOUNDS;
    }
    pin->index = pin_index;

    switch (direction) {
    case HAL_GPIO_INPUT:
        pin->direction = kGPIO_DigitalInput;
        break;
    case HAL_GPIO_OUTPUT:
        pin->direction = kGPIO_DigitalOutput;
        break;
    default:
        hal_unreachable();
    }

    switch (intr_mode) {
    case HAL_GPIO_INTR_DISABLED:
        pin->intr_mode = kGPIO_NoIntmode;
        break;
    case HAL_GPIO_INTR_LOW_LEVEL:
        pin->intr_mode = kGPIO_IntLowLevel;
        break;
    case HAL_GPIO_INTR_HIGH_LEVEL:
        pin->intr_mode = kGPIO_IntHighLevel;
        break;
    case HAL_GPIO_INTR_RISING_EDGE:
        pin->intr_mode = kGPIO_IntRisingEdge;
        break;
    case HAL_GPIO_INTR_FALLING_EDGE:
        pin->intr_mode = kGPIO_IntFallingEdge;
        break;
    case HAL_GPIO_INTR_RISING_OR_FALLING_EDGE:
        pin->intr_mode = kGPIO_IntRisingOrFallingEdge;
        break;
    default:
        hal_unreachable();
    }

    if ((*group_intrs_ptr(group, base_index) & pin_index) != 0) {
        // Interrupt already registered for this pin.
        return HAL_FAILURE;
    }

    gpio_pin_config_t config = {pin->direction, 0, pin->intr_mode};
    GPIO_PinInit(pin->base, pin->index, &config);

    pin_enable_intr(pin);

    return HAL_SUCCESS;
}

void hal_gpio_pin_deinit(HalGpioPin *pin) {
    pin_disable_intr(pin);

    gpio_pin_config_t config = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};
    GPIO_PinInit(pin->base, pin->index, &config);
}

bool hal_gpio_pin_read(const HalGpioPin *pin) {
    return GPIO_PinRead(pin->base, pin->index) != 0;
}

void hal_gpio_pin_write(const HalGpioPin *pin, bool value) {
    GPIO_PinWrite(pin->base, pin->index, (uint8_t)value);
}


static void handle_irq(_HalGpioBaseIndex base_index, bool upper) {
    GPIO_Type *base = BASES[base_index];
    HalGpioPinMask flags = GPIO_GetPinsInterruptFlags(base);
    HalGpioPinMask handled_flags = 0;
    for (size_t i = 0; i < GROUPS_COUNT; ++i) {
        HalGpioGroup *group = GROUPS[i];
        HalGpioPinMask intrs = group_intrs_half(group, base_index, upper);
        HalGpioPinMask group_flags = intrs & flags;
        if (group_flags) {
            void *user_data = group->user_data;
            HalGpioIntrCallback callback = group->callback;
            if (callback != NULL) {
                callback(user_data, base_index + _HAL_GPIO_BLOCK_START, group_flags);
            }
            handled_flags |= group_flags;
        }
    }
    GPIO_ClearPinsInterruptFlags(base, handled_flags);
    if (handled_flags == 0) {
        // Interrupt fired but no flags was handled
        hal_panic();
    }

    // Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
    // exception return operation might vector to incorrect interrupt
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}

// clang-format off
void GPIO1_Combined_0_15_IRQHandler() { handle_irq(0, false); }
void GPIO1_Combined_16_31_IRQHandler() { handle_irq(0, true); }
void GPIO2_Combined_0_15_IRQHandler() { handle_irq(1, false); }
void GPIO2_Combined_16_31_IRQHandler() { handle_irq(1, true); }
void GPIO3_Combined_0_15_IRQHandler() { handle_irq(2, false); }
void GPIO3_Combined_16_31_IRQHandler() { handle_irq(2, true); }
void GPIO4_Combined_0_15_IRQHandler() { handle_irq(3, false); }
void GPIO4_Combined_16_31_IRQHandler() { handle_irq(3, true); }
void GPIO5_Combined_0_15_IRQHandler() { handle_irq(4, false); }
void GPIO5_Combined_16_31_IRQHandler() { handle_irq(4, true); }
// clang-format on
