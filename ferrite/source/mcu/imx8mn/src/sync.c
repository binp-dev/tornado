#include "sync.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "fsl_iomuxc.h"

#include <hal/assert.h>
#include <hal/gpt.h>
#include <hal/gpio.h>

#include "stats.h"


#define SYN_10K_MUX IOMUXC_SAI3_TXC_GPIO5_IO00
// #define SYN_10K_MUX IOMUXC_SAI3_TXC_GPT1_COMPARE2
#define SYN_10K_PIN 5, 0

#define SYN_1_MUX IOMUXC_SAI3_RXD_GPIO4_IO30
// #define SYN_1_MUX IOMUXC_SAI3_RXD_GPT1_COMPARE1
#define SYN_1_PIN 4, 30

#define GPT_CHANNEL 1
#define GPT_PERIOD_US 1000 // 100


static void handle_gpt(void *data) {
    BaseType_t hptw = pdFALSE;
    SemaphoreHandle_t *sem = (SemaphoreHandle_t *)data;

    // Notify target task
    xSemaphoreGiveFromISR(*sem, &hptw);

    // Yield to higher priority task
    portYIELD_FROM_ISR(hptw);
}

void sync_generator_task(void *param) {
    hal_log_info("GPT init");

    HalGpt gpt;
    hal_assert(hal_gpt_init(&gpt, 1) == HAL_SUCCESS);

    IOMUXC_SetPinMux(SYN_10K_MUX, 0u);
    IOMUXC_SetPinMux(SYN_1_MUX, 0u);

    HalGpioGroup group;
    hal_gpio_group_init(&group);
    HalGpioPin gpt_pins[2];
    hal_gpio_pin_init(&gpt_pins[0], &group, SYN_10K_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&gpt_pins[1], &group, SYN_1_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_write(&gpt_pins[0], false);

    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    hal_assert(sem != NULL);

    bool pin_state = false;
    hal_assert(hal_gpt_start(&gpt, GPT_CHANNEL, GPT_PERIOD_US / 2, handle_gpt, (void *)&sem) == HAL_SUCCESS);
    for (size_t i = 0;; ++i) {
        if (xSemaphoreTake(sem, 10000) != pdTRUE) {
            hal_log_info("GPT semaphore timeout %x", i);
            continue;
        }

        // Toggle pin
        pin_state = !pin_state;
        hal_gpio_pin_write(&gpt_pins[0], pin_state);

        if (pin_state) {
            STATS.clock_count += 1;
        }
    }

    hal_log_error("End of task_gpt()");
    hal_panic();

    hal_assert(hal_gpt_stop(&gpt) == HAL_SUCCESS);
    hal_assert(hal_gpt_deinit(&gpt) == HAL_SUCCESS);
}
