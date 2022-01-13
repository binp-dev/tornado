#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "rsc_table.h"

#include <stdint.h>
#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <ipp.h>

#include <hal/assert.h>
#include <hal/io.h>
#include <hal/rpmsg.h>
#include <hal/math.h>
#include <hal/time.h>
#include <hal/gpt.h>
#include <hal/gpio.h>

#include "skifio.h"

#define TASK_STACK_SIZE 256

#define SMP_RDY_PIN 5, 23
#define READ_RDY_PIN 5, 9

#define DAC_KEY_0 5, 2
#define DAC_KEY_1 5, 3

#define GPIO_PIN_0 5, 0
#define GPIO_PIN_1 4, 30

static volatile SemaphoreHandle_t smp_rdy_sem = NULL;

static volatile int32_t g_dac = 0;
static volatile int64_t g_adcs[SKIFIO_ADC_CHANNEL_COUNT] = {0};
static volatile uint32_t g_sample_count = 0;

static volatile uint32_t intr_count = 0;

void smp_rdy_handler(void *user_data, HalGpioBlockIndex block, HalGpioPinMask mask) {
    intr_count += 1;

    BaseType_t hptw = pdFALSE;

    // Notify target task
    xSemaphoreGiveFromISR(smp_rdy_sem, &hptw);

    // Yield to higher priority task
    portYIELD_FROM_ISR(hptw);
}

static void task_gpt(void *param) {
    hal_log_info("GPT init");
    hal_assert(hal_gpt_init(0) == HAL_SUCCESS);

    BOARD_InitGptPins();
    HalGpioGroup group;
    hal_gpio_group_init(&group);
    HalGpioPin gpt_pins[2];
    hal_gpio_pin_init(&gpt_pins[0], &group, GPIO_PIN_0, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&gpt_pins[1], &group, GPIO_PIN_1, HAL_GPIO_INPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_write(&gpt_pins[0], false);

    SemaphoreHandle_t gpt_sem = xSemaphoreCreateBinary();
    hal_assert(gpt_sem != NULL);

    hal_assert(hal_gpt_start(0, 1000, gpt_sem) == HAL_SUCCESS);

    for (size_t i = 0;;++i) {
        if (xSemaphoreTake(gpt_sem, 10000) != pdTRUE) {
            hal_log_info("GPT semaphore timeout %x", i);
            continue;
        }
        //hal_log_info("GPT tick: %d", i);

        hal_gpio_pin_write(&gpt_pins[0], true);
        hal_busy_wait_ns(100000ll);
        hal_gpio_pin_write(&gpt_pins[0], false);
    }

    hal_log_error("End of task_gpio()");
    hal_panic();

    hal_assert(hal_gpt_stop(0) == HAL_SUCCESS);
    hal_assert(hal_gpt_deinit(0) == HAL_SUCCESS);
}

static void task_gpio(void *param) {
    hal_log_info("SkifIO driver init");
    hal_assert(skifio_init() == HAL_SUCCESS);

    hal_log_info("GPIO init");

    BOARD_InitGpioPins();

    HalGpioGroup group;
    hal_gpio_group_init(&group);

    // DAC keys
    HalGpioPin dac_keys[2];
    hal_gpio_pin_init(&dac_keys[0], &group, DAC_KEY_0, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_init(&dac_keys[1], &group, DAC_KEY_1, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_write(&dac_keys[0], true);
    hal_gpio_pin_write(&dac_keys[1], true);

    HalGpioPin read_rdy;
    hal_gpio_pin_init(&read_rdy, &group, READ_RDY_PIN, HAL_GPIO_OUTPUT, HAL_GPIO_INTR_DISABLED);
    hal_gpio_pin_write(&read_rdy, false);

    smp_rdy_sem = xSemaphoreCreateBinary();
    hal_assert(smp_rdy_sem != NULL);

    HalGpioPin smp_rdy;
    hal_gpio_pin_init(&smp_rdy, &group, SMP_RDY_PIN, HAL_GPIO_INPUT, HAL_GPIO_INTR_FALLING_EDGE);
    hal_gpio_group_set_intr(&group, smp_rdy_handler, NULL);

    TickType_t meas_start = xTaskGetTickCount();
    hal_busy_wait_ns(1000000000ll);
    hal_log_info("ms per 1e9 busy loop ns: %d", xTaskGetTickCount() - meas_start);

    hal_log_info("Enter GPIO loop");

    // Statistics
    size_t prev_intr_count = 0;
    size_t max_intr_count = 0;
    int32_t min_adc = 0;
    int32_t max_adc = 0;
    int32_t last_adcs[SKIFIO_ADC_CHANNEL_COUNT] = {0};

    TickType_t last_ticks = 0; 
    SkifioInput input = {{0}};
    SkifioOutput output = {0};
    for (size_t i = 0;;++i) {
        hal_retcode ret;

        if (xSemaphoreTake(smp_rdy_sem, 1000) != pdTRUE) {
            hal_log_info("GPIO semaphore timeout %x", i);
            continue;
        }
        max_intr_count = hal_max(max_intr_count, intr_count - prev_intr_count);
        prev_intr_count = intr_count;

        // Wait before data request to reduce ADC noise.
        //vTaskDelay(1);
        hal_busy_wait_ns(100000);

        output.dac = (int16_t)g_dac;
        ret = skifio_transfer(&output, &input);
        hal_assert(ret == HAL_SUCCESS || ret == HAL_INVALID_DATA); // Ignore CRC check error
        for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
            volatile int64_t *accum = &g_adcs[j];
            int32_t value = input.adcs[j];

            if (g_sample_count == 0) {
                *accum = value;
            } else {
                *accum += value;
            }

            min_adc = hal_min(min_adc, value);
            max_adc = hal_max(max_adc, value);
            last_adcs[j] = value;
        }
        g_sample_count += 1;

        hal_gpio_pin_write(&read_rdy, true);
        //vTaskDelay(1);
        hal_busy_wait_ns(10000);
        hal_gpio_pin_write(&read_rdy, false);

        if (xTaskGetTickCount() - last_ticks >= 1000) {
            hal_log_info("max_intr_count: %d", max_intr_count);
            hal_log_info("min_adc: 0x%x, max_adc: 0x%x", min_adc, max_adc);
            max_intr_count = 0;
            min_adc = 0;
            max_adc = 0;
            for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
                hal_log_info("adc%d: %x", j, last_adcs[j]);
            }
            last_ticks = xTaskGetTickCount();

            // To skip interrupts occured while printing debug info.
            xSemaphoreTake(smp_rdy_sem, 0);
            prev_intr_count = intr_count;
        }
    }

    hal_log_error("End of task_gpio()");
    hal_panic();

    hal_assert(skifio_deinit() == HAL_SUCCESS);
}

static void task_rpmsg(void *param) {
    hal_rpmsg_init();

    hal_rpmsg_channel channel;
    hal_assert(hal_rpmsg_create_channel(&channel, 0) == HAL_SUCCESS);
#ifdef HAL_PRINT_RPMSG
    hal_io_rpmsg_init(&channel);
#endif
    hal_log_info("RPMSG channel created");

    // Receive message

    uint8_t *buffer = NULL;
    size_t len = 0;
    hal_rpmsg_recv_nocopy(&channel, &buffer, &len, HAL_WAIT_FOREVER);
    hal_assert(strncmp((const char *)buffer, "hello world!", len) == 0);
    hal_log_info("hello world!");
    hal_rpmsg_free_rx_buffer(&channel, buffer);
    buffer = NULL;
    len = 0;

    // Start messaging

    const IppAppMsg *app_msg = NULL;
    hal_rpmsg_recv_nocopy(&channel, &buffer, &len, HAL_WAIT_FOREVER);
    app_msg = (const IppAppMsg *)buffer;
    if (app_msg->type == IPP_APP_MSG_START) {
        hal_log_info("Start message received");
    } else {
        hal_log_error("Message error: type mismatch: %d", (int)app_msg->type);
        hal_panic();
    }
    hal_rpmsg_free_rx_buffer(&channel, buffer);
    buffer = NULL;
    len = 0;

    hal_log_info("Enter RPMSG loop");

    for (;;) {
        // Receive message
        hal_assert(hal_rpmsg_recv_nocopy(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
        app_msg = (const IppAppMsg *)buffer;
        //hal_log_info("Received message: 0x%02x", (int)app_msg->type);

        switch (app_msg->type) {
        case IPP_APP_MSG_DAC_SET:
            g_dac = app_msg->dac_set.value;
            //hal_log_info("Write DAC value: %x", g_dac);
            hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
            break;
        case IPP_APP_MSG_ADC_REQ:
            //hal_log_info("Read ADC values");
            hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
            hal_assert(hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
            IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
            mcu_msg->type = IPP_MCU_MSG_ADC_VAL;
            for (size_t i = 0; i < SKIFIO_ADC_CHANNEL_COUNT; ++i) {
                volatile int64_t *accum = &g_adcs[i];
                if (g_sample_count > 0) {
                    *accum /= g_sample_count;
                }
                mcu_msg->adc_val.values.data[i] = (int32_t)(*accum);
            }
            g_sample_count = 0;
            hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);
            break;

        default:
            hal_log_error("Wrong message type: %d", (int)app_msg->type);
            hal_panic();
        }   
    }

    hal_log_error("End of task_rpmsg()");
    hal_panic();

    // FIXME: Should never reach this point - otherwise virtio hangs
    hal_assert(hal_rpmsg_destroy_channel(&channel) == HAL_SUCCESS);
    
    hal_rpmsg_deinit();
}

/*!
 * @brief Main function
 */
int main(void)
{
    /* Initialize standard SDK demo application pins */
    /* M7 has its local cache and enabled by default,
     * need to set smart subsystems (0x28000000 ~ 0x3FFFFFFF)
     * non-cacheable before accessing this address region */
    BOARD_InitMemory();

    /* Board specific RDC settings */
    BOARD_RdcInit();

    BOARD_InitBootPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    copyResourceTable();

#ifdef MCMGR_USED
    /* Initialize MCMGR before calling its API */
    (void)MCMGR_Init();
#endif /* MCMGR_USED */

        // Create GPT task.
    hal_log_info("Create GPT task");
    xTaskCreate(
        task_gpt, "GPT task",
        TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL
    );

    // Create GPIO task.
    hal_log_info("Create GPIO task");
    xTaskCreate(
        task_gpio, "GPIO task",
        TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    /* Create RPMSG task. */
    hal_log_info("Create RPMSG task");
    xTaskCreate(
        task_rpmsg, "RPMSG task",
        TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL
    );

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    hal_log_error("End of main()");
    hal_panic();

    return 0;
}
