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
#include <stream_buffer.h>

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

// TODO: Check values
#define MAX_POINTS_IN_RPMSG                         63 
#define DAC_WF_PV_SIZE                              10000
#define DAC_WF_BUFF_SIZE                            1000
#define ADC_WF_BUFF_SIZE                            (MAX_POINTS_IN_RPMSG*5)
#define FREE_SPACE_IN_BUFF_FOR_DAC_WF_REQUEST       (MAX_POINTS_IN_RPMSG)

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
static SemaphoreHandle_t rpmsg_send_sem = NULL;

typedef struct {
    StreamBufferHandle_t buff;
    bool was_set;
    bool waiting_for_data;
} DacWf;

static volatile DacWf dac_wf;
static volatile StreamBufferHandle_t adc_wf_buff[SKIFIO_ADC_CHANNEL_COUNT];
static volatile bool ioc_started = false;

static void din_handler(void *data, SkifioDin value) {
    ACCUM.din = value;
    BaseType_t hptw = pdFALSE;
    xSemaphoreGiveFromISR(rpmsg_send_sem, &hptw);
    portYIELD_FROM_ISR(hptw);
}


void send_adc_wf_data() {
    for (int i = 0; i < SKIFIO_ADC_CHANNEL_COUNT; ++i) {
        size_t elems_in_buff = xStreamBufferBytesAvailable(adc_wf_buff[i]) / sizeof(int32_t);
        if (elems_in_buff >= MAX_POINTS_IN_RPMSG) {
            uint8_t *buffer = NULL;
            size_t len = 0;
            hal_assert(hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
            IppMcuMsg *adc_wf_msg = (IppMcuMsg *)buffer;
            adc_wf_msg->type = IPP_MCU_MSG_ADC_WF;
            adc_wf_msg->adc_wf.index = i;

            size_t sent_data_size = xStreamBufferReceive(
                adc_wf_buff[i],
                &(adc_wf_msg->adc_wf.elements.data[0]),
                MAX_POINTS_IN_RPMSG*sizeof(int32_t),
                0
            );
            adc_wf_msg->adc_wf.elements.len = sent_data_size/sizeof(int32_t); 
            hal_assert(sent_data_size/sizeof(int32_t) == MAX_POINTS_IN_RPMSG); // TODO: Delete after debug?

            hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(adc_wf_msg)) == HAL_SUCCESS);
        }
    }
}

void send_adc_wf_request() {
    size_t free_space_in_buff = xStreamBufferSpacesAvailable(dac_wf.buff) / sizeof(int32_t);

    if (free_space_in_buff >= FREE_SPACE_IN_BUFF_FOR_DAC_WF_REQUEST && !dac_wf.waiting_for_data) {
        dac_wf.waiting_for_data = true;
        uint8_t *buffer = NULL;
        size_t len = 0;
        hal_assert(hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);

        IppMcuMsg *dac_wf_req_msg = (IppMcuMsg *)buffer;
        dac_wf_req_msg->type = IPP_MCU_MSG_DAC_WF_REQ;

        hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(dac_wf_req_msg)) == HAL_SUCCESS);
    }
}

void send_din() {
        SkifioDin din = skifio_din_read();
        if (din == ACCUM.din) {
            return;
        }

        ACCUM.din = din;

        uint8_t *buffer = NULL;
        size_t len = 0;
        hal_assert(hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
        IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
        mcu_msg->type = IPP_MCU_MSG_DIN_VAL;
        mcu_msg->din_val.value = ACCUM.din;
        hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);
}

static void task_rpmsg_send(void *param) {
    for (;;) {
        if (!ioc_started) {
            continue;
        }
        xSemaphoreTake(rpmsg_send_sem, portMAX_DELAY);

        send_din();
        send_adc_wf_data();
        send_adc_wf_request();

        vTaskDelay(10);
    }
}

static void task_skifio(void *param) {
    TickType_t meas_start = xTaskGetTickCount();
    hal_busy_wait_ns(1000000000ll);
    hal_log_info("ms per 1e9 busy loop ns: %ld", xTaskGetTickCount() - meas_start);

    hal_log_info("SkifIO driver init");
    hal_assert(skifio_init() == HAL_SUCCESS);

    hal_log_info("Create rpmsg_send task");
    xTaskCreate(task_rpmsg_send, "rpmsg_send task", TASK_STACK_SIZE, NULL, RPMSG_TASK_PRIORITY, NULL);
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
        bool need_send_din = false;
        if (din != ACCUM.din) {
            need_send_din = true;
        }

        STATS.max_intrs_per_sample = hal_max(
            STATS.max_intrs_per_sample,
            (uint32_t)(_SKIFIO_DEBUG_INFO.intr_count - prev_intr_count)
        );
        prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;

        int32_t dac_wf_value = 0;   // TODO: set zero in Volts, not in code
        if (dac_wf.was_set) {
            size_t readed_data_size = xStreamBufferReceive(dac_wf.buff, &dac_wf_value, sizeof(int32_t), 0);
            hal_assert(readed_data_size % sizeof(int32_t) == 0); // TODO: Delete after debug?
            if (readed_data_size == 0) {
                ++STATS.dac_wf.buff_was_empty;
            }
        }
        
        size_t free_space_in_buff = xStreamBufferSpacesAvailable(dac_wf.buff) / sizeof(int32_t);
        bool need_dac_wf_req = false;
        if (free_space_in_buff >= FREE_SPACE_IN_BUFF_FOR_DAC_WF_REQUEST) {
            need_dac_wf_req = true;
        }

        output.dac = (int16_t)dac_wf_value;

        ret = skifio_transfer(&output, &input);
        hal_assert(ret == HAL_SUCCESS || ret == HAL_INVALID_DATA); // Ignore CRC check error

        bool need_send_adc = false;

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

            if (ioc_started) {
                size_t added_data_size = xStreamBufferSend(adc_wf_buff[j], &value, sizeof(int32_t), 0);
                hal_assert(added_data_size % sizeof(int32_t) == 0); // TODO: Delete after debug?
                if (added_data_size == 0) {
                    ++STATS.adc_buff_was_full[j];
                }

                size_t elems_in_buff = xStreamBufferBytesAvailable(adc_wf_buff[j]) / sizeof(int32_t);
                if (elems_in_buff >= MAX_POINTS_IN_RPMSG) {
                    need_send_adc = true;
                }
            }
        }

        if (need_send_din || need_send_adc || need_dac_wf_req) {
            xSemaphoreGive(rpmsg_send_sem);
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
        ioc_started = true;
        xSemaphoreGive(rpmsg_send_sem);
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
            ioc_started = true;
            xSemaphoreGive(rpmsg_send_sem);
            hal_log_info("MCU program is already started");
            hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
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
            hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
            break;

        }
        case IPP_APP_MSG_DAC_WF: {
            size_t added_data_size = xStreamBufferSend(dac_wf.buff, app_msg->dac_wf.elements.data, app_msg->dac_wf.elements.len * sizeof(int32_t), 0);
            hal_assert(added_data_size % sizeof(int32_t) == 0); // TODO: Delete after debug?
            dac_wf.was_set = true;

            if (added_data_size/sizeof(int32_t) != app_msg->dac_wf.elements.len) {
                ++STATS.dac_wf.buff_was_full;
            }

            dac_wf.waiting_for_data = false;
            hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
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

void initialize_wf_buffers() {
    dac_wf.waiting_for_data = false;
    dac_wf.was_set = false;
    dac_wf.buff = xStreamBufferCreate(DAC_WF_BUFF_SIZE*sizeof(int32_t), 0);
    if (dac_wf.buff == NULL) {
        hal_log_error("Can't initialize buffer for output waveform");
        hal_panic();
    }

    for (size_t i = 0; i < SKIFIO_ADC_CHANNEL_COUNT; ++i) {
        adc_wf_buff[i] = xStreamBufferCreate(ADC_WF_BUFF_SIZE*sizeof(int32_t), 0);
        if (adc_wf_buff[i] == NULL) {
            hal_log_error("Can't initialize buffer for input waveform");
            hal_panic();
        }
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

    hal_io_uart_init(3);

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

    initialize_wf_buffers();
    rpmsg_send_sem = xSemaphoreCreateBinary();
    hal_assert(rpmsg_send_sem != NULL);

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
