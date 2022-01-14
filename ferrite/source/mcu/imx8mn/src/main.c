#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "rsc_table.h"
#include "fsl_common.h"
#include "fsl_iomuxc.h"

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

#define SYN_10K_MUX IOMUXC_SAI3_TXC_GPIO5_IO00
// #define SYN_10K_MUX IOMUXC_SAI3_TXC_GPT1_COMPARE2
#define SYN_10K_PIN 5, 0

#define SYN_1_MUX IOMUXC_SAI3_RXD_GPIO4_IO30
// #define SYN_1_MUX IOMUXC_SAI3_RXD_GPT1_COMPARE1
#define SYN_1_PIN 4, 30

#define GPT_CHANNEL 1
#define GPT_PERIOD_US 10000

typedef struct {
    int32_t dac;
    int64_t adcs[SKIFIO_ADC_CHANNEL_COUNT];
    uint32_t sample_count;
} Accum;

typedef struct {
    uint32_t clock_count;
    uint32_t sample_count;
    uint32_t max_intrs_per_sample;
    int32_t min_adc;
    int32_t max_adc;
    int32_t last_adcs[SKIFIO_ADC_CHANNEL_COUNT];
} Statistics;

static volatile Accum ACCUM = {0, {0}, 0};
static volatile Statistics STATS = {0, 0, 0, 0, 0, {0}};

static void handle_gpt(void *data) {
    BaseType_t hptw = pdFALSE;
    SemaphoreHandle_t *sem = (SemaphoreHandle_t *)data;

    // Notify target task
    xSemaphoreGiveFromISR(*sem, &hptw);

    // Yield to higher priority task
    portYIELD_FROM_ISR(hptw);
}

static void task_gpt(void *param) {
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
    for (size_t i = 0;;++i) {
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

static void task_skifio(void *param) {
    TickType_t meas_start = xTaskGetTickCount();
    hal_busy_wait_ns(1000000000ll);
    hal_log_info("ms per 1e9 busy loop ns: %ld", xTaskGetTickCount() - meas_start);

    hal_log_info("SkifIO driver init");
    hal_assert(skifio_init() == HAL_SUCCESS);

    SkifioInput input = {{0}};
    SkifioOutput output = {0};

    hal_log_info("Enter SkifIO loop");
    uint64_t prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;
    for (size_t i = 0;;++i) {
        hal_retcode ret;

        ret = skifio_wait_ready(1000);
        if (ret == HAL_TIMED_OUT) {
            hal_log_info("SkifIO timeout %d", i);
            continue;
        }
        hal_assert(ret == HAL_SUCCESS);


        STATS.max_intrs_per_sample = hal_max(
            STATS.max_intrs_per_sample,
            (uint32_t)(_SKIFIO_DEBUG_INFO.intr_count - prev_intr_count) //
        );
        prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;

        output.dac = (int16_t)ACCUM.dac;

        ret = skifio_transfer(&output, &input);
        hal_assert(ret == HAL_SUCCESS || ret == HAL_INVALID_DATA); // Ignore CRC check error

        for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
            volatile int64_t *accum = &ACCUM.adcs[j];
            int32_t value = input.adcs[j];

            if (ACCUM.sample_count == 0) {
                *accum = value;
            } else {
                *accum += value;
            }

            STATS.min_adc = hal_min(STATS.min_adc, value);
            STATS.max_adc = hal_max(STATS.max_adc, value);
            STATS.last_adcs[j] = value;
        }
        ACCUM.sample_count += 1;
        STATS.sample_count += 1;
    }

    hal_log_error("End of task_skifio()");
    hal_panic();

    hal_assert(skifio_deinit() == HAL_SUCCESS);
}

static void task_stats(void *param) {
    for (;;) {
        hal_log_info("");
        hal_log_info("clock_count: %ld", STATS.clock_count);
        hal_log_info("sample_count: %ld", STATS.sample_count);
        hal_log_info("max_intrs_per_sample: %ld", STATS.max_intrs_per_sample);
        int32_t v_min = STATS.min_adc;
        hal_log_info("min_adc: (0x%08lx) %ld", v_min, v_min);
        int32_t v_max = STATS.max_adc;
        hal_log_info("max_adc: (0x%08lx) %ld", v_max, v_max);

        STATS.clock_count = 0;
        STATS.sample_count = 0;
        STATS.max_intrs_per_sample = 0;
        STATS.min_adc = 0;
        STATS.max_adc = 0;

        for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
            int32_t v = STATS.last_adcs[j];
            hal_log_info("adc%d: (0x%08lx) %ld", j, v, v);
        }

        vTaskDelay(1000);
    }
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
            ACCUM.dac = app_msg->dac_set.value;
            // hal_log_info("Write DAC value: %x", ACCUM.dac);
            hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
            break;
        case IPP_APP_MSG_ADC_REQ:
            //hal_log_info("Read ADC values");
            hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
            hal_assert(hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
            IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
            mcu_msg->type = IPP_MCU_MSG_ADC_VAL;
            for (size_t i = 0; i < SKIFIO_ADC_CHANNEL_COUNT; ++i) {
                volatile int64_t *accum = &ACCUM.adcs[i];
                if (ACCUM.sample_count > 0) {
                    *accum /= ACCUM.sample_count;
                }
                mcu_msg->adc_val.values.data[i] = (int32_t)(*accum);
            }
            ACCUM.sample_count = 0;
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
#endif
    hal_log_info("\n\r\n\r** Board started **");

    hal_log_info("Create GPT task");
    xTaskCreate(task_gpt, "GPT task", TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);

    hal_log_info("Create SkifIO task");
    xTaskCreate(task_skifio, "SkifIO task", TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);

    hal_log_info("Create RPMsg task");
    xTaskCreate(task_rpmsg, "RPMsg task", TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    hal_log_info("Create statistics task");
    xTaskCreate(task_stats, "Statistics task", TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    hal_log_error("End of main()");
    hal_panic();

    return 0;
}
