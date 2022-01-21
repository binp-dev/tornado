#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "rsc_table.h"
#include "fsl_common.h"
#include "fsl_gpio.h"

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

#include "skifio.h"
#include "stats.h"

#ifdef GENERATE_SYNC
#include "sync.h"
#endif


#define TASK_STACK_SIZE 256
#define RPMSG_TASK_PRIORITY tskIDLE_PRIORITY + 1

typedef struct {
    int32_t last;
    int64_t sum;
} AdcAccum;

typedef struct {
    int32_t dac;
    AdcAccum adcs[SKIFIO_ADC_CHANNEL_COUNT];
    uint32_t sample_count;
    SkifioDin din;
    SkifioDout dout;
} Accum;

static volatile Accum ACCUM = {0, {{0, 0}}, 0, 0, 0};

static hal_rpmsg_channel channel;
static SemaphoreHandle_t din_sem = NULL;

static void din_handler(void *data, SkifioDin value) {
    ACCUM.din = value;
    BaseType_t hptw = pdFALSE;
    xSemaphoreGiveFromISR(din_sem, &hptw);
    portYIELD_FROM_ISR(hptw);
}

static void task_rpmsg_send_din(void *param) {
    for (;;) {
        xSemaphoreTake(din_sem, portMAX_DELAY);
        ACCUM.din = skifio_din_read();
        hal_log_info("din changed: %lx", (uint32_t)ACCUM.din);

        uint8_t *buffer = NULL;
        size_t len = 0;
        hal_assert(hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
        IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
        mcu_msg->type = IPP_MCU_MSG_DIN_VAL;
        mcu_msg->din_val.value = ACCUM.din;
        hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);

        vTaskDelay(10);
    }
}

static void task_skifio(void *param) {
    TickType_t meas_start = xTaskGetTickCount();
    hal_busy_wait_ns(1000000000ll);
    hal_log_info("ms per 1e9 busy loop ns: %ld", xTaskGetTickCount() - meas_start);

    hal_log_info("SkifIO driver init");
    hal_assert(skifio_init() == HAL_SUCCESS);

    hal_log_info("Create rpmsg_send_din task");
    xTaskCreate(task_rpmsg_send_din, "rpmsg_send_din task", TASK_STACK_SIZE, NULL, RPMSG_TASK_PRIORITY, NULL);
    hal_assert(skifio_din_subscribe(din_handler, NULL) == HAL_SUCCESS);

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

        SkifioDin din = skifio_din_read();
        if (din != ACCUM.din) {
            ACCUM.din = din;
            xSemaphoreGive(din_sem);
        }

        STATS.max_intrs_per_sample = hal_max(
            STATS.max_intrs_per_sample,
            (uint32_t)(_SKIFIO_DEBUG_INFO.intr_count - prev_intr_count) //
        );
        prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;

        output.dac = (int16_t)ACCUM.dac;

        ret = skifio_transfer(&output, &input);
        hal_assert(ret == HAL_SUCCESS || ret == HAL_INVALID_DATA); // Ignore CRC check error

        for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
            volatile AdcAccum *accum = &ACCUM.adcs[j];
            volatile AdcStats *stats = &STATS.adcs[j];
            int32_t value = input.adcs[j];

            accum->last = value;
            accum->sum += value;

            if (STATS.sample_count == 0) {
                stats->min = value;
                stats->max = value;
            } else {
                stats->min = hal_min(stats->min, value);
                stats->max = hal_max(stats->max, value);
            }
            stats->last = value;
            stats->sum += value;
        }
        ACCUM.sample_count += 1;
        STATS.sample_count += 1;
    }

    hal_log_error("End of task_skifio()");
    hal_panic();

    hal_assert(skifio_deinit() == HAL_SUCCESS);
}

static void task_rpmsg_recv(void *param) {
    hal_rpmsg_init();

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
        xSemaphoreGive(din_sem);
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
        case IPP_APP_MSG_START:
            xSemaphoreGive(din_sem);
            hal_log_info("MCU program is already started");
            break;

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
                volatile AdcAccum *accum = &ACCUM.adcs[i];
                int32_t value = 0;
#ifdef AVERAGE_ADC
                value = (int32_t)(accum->sum / ACCUM.sample_count);
#else
                value = accum->last;
#endif
                accum->sum = 0;
                mcu_msg->adc_val.values.data[i] = value;
            }
            ACCUM.sample_count = 0;
            hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);
            break;

        case IPP_APP_MSG_DOUT_SET: {
            SkifioDout mask = (SkifioDout)((1 << SKIFIO_DOUT_SIZE) - 1);
            SkifioDout value = app_msg->dout_set.value;
            if (~mask & value) {
                hal_log_warn("dout is out of bounds: %lx", (uint32_t)value);
            }
            ACCUM.dout = value & mask;
            // hal_log_info("Dout write: 0x%lx", (uint32_t)ACCUM.dout);
            hal_assert(skifio_dout_write(ACCUM.dout) == HAL_SUCCESS);
            break;
        }
        default:
            hal_log_error("Wrong message type: %d", (int)app_msg->type);
            continue;
        }
    }

    hal_log_error("End of task_rpmsg()");
    hal_panic();

    // FIXME: Should never reach this point - otherwise virtio hangs
    hal_assert(hal_rpmsg_destroy_channel(&channel) == HAL_SUCCESS);
    
    hal_rpmsg_deinit();
}

static void task_stats(void *param) {
    for (size_t i = 0;;++i) {
        hal_log_info("");
        stats_print();
        stats_reset();
        hal_log_info("din: 0x%02lx", (uint32_t)ACCUM.din);
        hal_log_info("dout: 0x%01lx", (uint32_t)ACCUM.dout);
        vTaskDelay(10000);
    }
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

    hal_io_uart_init(4);

    copyResourceTable();

#ifdef MCMGR_USED
    /* Initialize MCMGR before calling its API */
    (void)MCMGR_Init();
#endif
    hal_log_info("\n\r\n\r** Board started **");

#ifdef GENERATE_SYNC
    hal_log_info("Create sync generator task");
    xTaskCreate(sync_generator_task, "Sync generator task", TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 4, NULL);
#endif

    din_sem = xSemaphoreCreateBinary();
    hal_assert(din_sem != NULL);

    hal_log_info("Create SkifIO task");
    xTaskCreate(task_skifio, "SkifIO task", TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);

    hal_log_info("Create RPMsg task");
    xTaskCreate(task_rpmsg_recv, "RPMsg task", TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);

    hal_log_info("Create statistics task");
    xTaskCreate(task_stats, "Statistics task", TASK_STACK_SIZE, NULL, RPMSG_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    hal_log_error("End of main()");
    hal_panic();

    return 0;
}
