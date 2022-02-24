#include "board.h"
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

#include <config.h>
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

#ifdef GENERATE_SYNC
#define SYNC_TASK_PRIORITY tskIDLE_PRIORITY + 4
#endif
#define SKIFIO_TASK_PRIORITY tskIDLE_PRIORITY + 3
#define RPMSG_TASK_PRIORITY tskIDLE_PRIORITY + 2
#define STATS_TASK_PRIORITY tskIDLE_PRIORITY + 1

typedef int32_t point_t;

#define DAC_MAX_MSG_POINTS ((RPMSG_MAX_MSG_LEN - sizeof(((IppMcuMsg *)NULL)->type) - sizeof(IppAppMsgDacWf)) / sizeof(point_t))

// TODO: Check values
#define MAX_POINTS_IN_RPMSG 63
#define DAC_WF_PV_SIZE 10000
#define DAC_WF_BUFF_SIZE 1000
#define ADC_WF_BUFF_SIZE (MAX_POINTS_IN_RPMSG * 5)
#define FREE_SPACE_IN_BUFF_FOR_DAC_WF_REQUEST (MAX_POINTS_IN_RPMSG)

typedef struct {
    StreamBufferHandle_t queue;
    bool was_set;
    bool waiting_for_data;
} DacWf;

typedef struct {
    StreamBufferHandle_t queue;
} AdcWf;

typedef struct {
    SkifioDin in;
    SkifioDout out;
} LogicalIo;

static volatile LogicalIo DIO = {0, 0};

static volatile DacWf DAC = {NULL, 0};
static volatile AdcWf ADCS[SKIFIO_ADC_CHANNEL_COUNT] = {{NULL}};

static hal_rpmsg_channel RPMSG_CHANNEL;
static SemaphoreHandle_t RPMSG_SEND_SEM = NULL;

bool IOC_STARTED = false;

/*
static void din_handler(void *data, SkifioDin value) {
    DIO.in = value;
    BaseType_t hptw = pdFALSE;
    xSemaphoreGiveFromISR(RPMSG_SEND_SEM, &hptw);
    portYIELD_FROM_ISR(hptw);
}
*/

void send_adc_wf_data() {
    for (int i = 0; i < SKIFIO_ADC_CHANNEL_COUNT; ++i) {
        size_t elems_in_buff = xStreamBufferBytesAvailable(ADCS[i].queue) / sizeof(int32_t);
        if (elems_in_buff >= MAX_POINTS_IN_RPMSG) {
            uint8_t *buffer = NULL;
            size_t len = 0;
            hal_assert(hal_rpmsg_alloc_tx_buffer(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
            IppMcuMsg *adc_wf_msg = (IppMcuMsg *)buffer;
            adc_wf_msg->type = IPP_MCU_MSG_ADC_WF;
            adc_wf_msg->adc_wf.index = i;

            size_t sent_data_size = xStreamBufferReceive(
                ADCS[i].queue,
                &(adc_wf_msg->adc_wf.elements.data[0]),
                MAX_POINTS_IN_RPMSG * sizeof(int32_t),
                0);
            adc_wf_msg->adc_wf.elements.len = sent_data_size / sizeof(int32_t);
            // hal_log_debug("Sent waveform of size: %d", sent_data_size / sizeof(int32_t));
            hal_assert(sent_data_size / sizeof(int32_t) == MAX_POINTS_IN_RPMSG); // TODO: Delete after debug?

            hal_assert(hal_rpmsg_send_nocopy(&RPMSG_CHANNEL, buffer, ipp_mcu_msg_size(adc_wf_msg)) == HAL_SUCCESS);
        }
    }
}

void send_adc_wf_request() {
    size_t free_space_in_buff = xStreamBufferSpacesAvailable(DAC.queue) / sizeof(int32_t);

    if (free_space_in_buff >= FREE_SPACE_IN_BUFF_FOR_DAC_WF_REQUEST && !DAC.waiting_for_data) {
        DAC.waiting_for_data = true;
        uint8_t *buffer = NULL;
        size_t len = 0;
        hal_assert(hal_rpmsg_alloc_tx_buffer(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);

        IppMcuMsg *dac_wf_req_msg = (IppMcuMsg *)buffer;
        dac_wf_req_msg->type = IPP_MCU_MSG_DAC_WF_REQ;

        hal_assert(hal_rpmsg_send_nocopy(&RPMSG_CHANNEL, buffer, ipp_mcu_msg_size(dac_wf_req_msg)) == HAL_SUCCESS);
    }
}

void send_din() {
    SkifioDin din = skifio_din_read();
    if (din == DIO.in) {
        return;
    }

    DIO.in = din;
    uint8_t *buffer = NULL;
    size_t len = 0;
    hal_assert(hal_rpmsg_alloc_tx_buffer(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
    IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
    mcu_msg->type = IPP_MCU_MSG_DIN_VAL;
    mcu_msg->din_val.value = DIO.in;
    hal_assert(hal_rpmsg_send_nocopy(&RPMSG_CHANNEL, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);
}

static void task_rpmsg_send(void *param) {
    for (;;) {
        if (!IOC_STARTED) {
            // FIXME:
            vTaskDelay(1000);
            continue;
        }
        xSemaphoreTake(RPMSG_SEND_SEM, portMAX_DELAY);

        send_din();
        send_adc_wf_data();
        send_adc_wf_request();
    }
}

static void task_skifio(void *param) {
    hal_log_info("SkifIO driver init");
    hal_assert(skifio_init() == HAL_SUCCESS);
    // hal_assert(skifio_din_subscribe(din_handler, NULL) == HAL_SUCCESS);

    SkifioInput input = {{0}};
    SkifioOutput output = {0};

    hal_log_info("Enter SkifIO loop");
    uint64_t prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;
    for (size_t i = 0;; ++i) {
        hal_retcode ret;
        bool need_send_din = false;
        bool need_send_adc = false;
        bool need_dac_wf_req = false;


        ret = skifio_wait_ready(1000);
        if (ret == HAL_TIMED_OUT) {
            hal_log_info("SkifIO timeout %d", i);
            continue;
        }
        hal_assert(ret == HAL_SUCCESS);

        SkifioDin din = skifio_din_read();
        if (din != DIO.in) {
            need_send_din = true;
        }

        STATS.max_intrs_per_sample = hal_max(
            STATS.max_intrs_per_sample,
            (uint32_t)(_SKIFIO_DEBUG_INFO.intr_count - prev_intr_count) //
        );
        prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;

        int32_t dac_wf_value = 0; // TODO: set zero in Volts, not in code
        if (DAC.was_set) {
            size_t read_data_size = xStreamBufferReceive(DAC.queue, &dac_wf_value, sizeof(int32_t), 0);
            hal_assert(read_data_size % sizeof(int32_t) == 0); // TODO: Delete after debug?
            if (read_data_size == 0) {
                ++STATS.dac_wf.buff_was_empty;
            }
        }

        size_t free_space_in_buff = xStreamBufferSpacesAvailable(DAC.queue) / sizeof(int32_t);
        if (free_space_in_buff >= FREE_SPACE_IN_BUFF_FOR_DAC_WF_REQUEST) {
            need_dac_wf_req = true;
        }

        output.dac = (int16_t)dac_wf_value;

        ret = skifio_transfer(&output, &input);
        hal_assert(ret == HAL_SUCCESS || ret == HAL_INVALID_DATA); // Ignore CRC check error


        for (size_t j = 0; j < SKIFIO_ADC_CHANNEL_COUNT; ++j) {
            volatile AdcStats *stats = &STATS.adcs[j];
            int32_t value = input.adcs[j];

            if (STATS.sample_count == 0) {
                stats->min = value;
                stats->max = value;
            } else {
                stats->min = hal_min(stats->min, value);
                stats->max = hal_max(stats->max, value);
            }
            stats->last = value;
            stats->sum += value;

            if (IOC_STARTED) {
                size_t added_data_size = xStreamBufferSend(ADCS[j].queue, &value, sizeof(int32_t), 0);
                hal_assert(added_data_size % sizeof(int32_t) == 0); // TODO: Delete after debug?
                if (added_data_size == 0) {
                    ++STATS.adc_buff_was_full[j];
                }

                size_t elems_in_buff = xStreamBufferBytesAvailable(ADCS[j].queue) / sizeof(int32_t);
                if (elems_in_buff >= MAX_POINTS_IN_RPMSG) {
                    need_send_adc = true;
                }
            }
        }

        if (need_send_din || need_send_adc || need_dac_wf_req) {
            xSemaphoreGive(RPMSG_SEND_SEM);
        }

        STATS.sample_count += 1;
    }

    hal_log_error("End of task_skifio()");
    hal_panic();

    hal_assert(skifio_deinit() == HAL_SUCCESS);
}

static void task_rpmsg_recv(void *param) {
    hal_rpmsg_init();

    hal_assert(hal_rpmsg_create_channel(&RPMSG_CHANNEL, 0) == HAL_SUCCESS);
#ifdef HAL_PRINT_RPMSG
    hal_io_rpmsg_init(&RPMSG_CHANNEL);
#endif
    hal_log_info("RPMSG channel created");

    // Receive message

    uint8_t *buffer = NULL;
    size_t len = 0;
    hal_rpmsg_recv_nocopy(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER);
    hal_assert(strncmp((const char *)buffer, "hello world!", len) == 0);
    hal_log_info("hello world!");
    hal_rpmsg_free_rx_buffer(&RPMSG_CHANNEL, buffer);
    buffer = NULL;
    len = 0;

    // Start messaging

    const IppAppMsg *app_msg = NULL;
    hal_rpmsg_recv_nocopy(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER);
    app_msg = (const IppAppMsg *)buffer;
    if (app_msg->type == IPP_APP_MSG_START) {
        hal_log_info("Start message received");
        IOC_STARTED = true;
        xSemaphoreGive(RPMSG_SEND_SEM);
    } else {
        hal_log_error("Message error: type mismatch: %d", (int)app_msg->type);
        hal_panic();
    }
    hal_rpmsg_free_rx_buffer(&RPMSG_CHANNEL, buffer);
    buffer = NULL;
    len = 0;

    hal_log_info("Enter RPMSG loop");

    for (;;) {
        // Receive message
        hal_assert(hal_rpmsg_recv_nocopy(&RPMSG_CHANNEL, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
        app_msg = (const IppAppMsg *)buffer;

        switch (app_msg->type) {
        case IPP_APP_MSG_START:
            IOC_STARTED = true;
            xSemaphoreGive(RPMSG_SEND_SEM);
            hal_log_info("MCU program is already started");
            hal_assert(hal_rpmsg_free_rx_buffer(&RPMSG_CHANNEL, buffer) == HAL_SUCCESS);
            break;

        case IPP_APP_MSG_DOUT_SET: {
            SkifioDout mask = (SkifioDout)((1 << SKIFIO_DOUT_SIZE) - 1);
            SkifioDout value = app_msg->dout_set.value;
            if (~mask & value) {
                hal_log_warn("dout is out of bounds: %lx", (uint32_t)value);
            }
            DIO.out = value & mask;
            // hal_log_info("Dout write: 0x%lx", (uint32_t)DIO.out);
            hal_assert(skifio_dout_write(DIO.out) == HAL_SUCCESS);
            hal_assert(hal_rpmsg_free_rx_buffer(&RPMSG_CHANNEL, buffer) == HAL_SUCCESS);
            break;
        }
        case IPP_APP_MSG_DAC_WF: {
            size_t added_data_size = xStreamBufferSend(
                DAC.queue,
                app_msg->dac_wf.elements.data,
                app_msg->dac_wf.elements.len * sizeof(int32_t),
                0);
            hal_assert(added_data_size % sizeof(int32_t) == 0);
            DAC.was_set = true;

            if (added_data_size / sizeof(int32_t) != app_msg->dac_wf.elements.len) {
                hal_log_error("Not enough space in dac waveform buffer to save new data");
                ++STATS.dac_wf.buff_was_full;
            }

            DAC.waiting_for_data = false;
            hal_assert(hal_rpmsg_free_rx_buffer(&RPMSG_CHANNEL, buffer) == HAL_SUCCESS);
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
    hal_assert(hal_rpmsg_destroy_channel(&RPMSG_CHANNEL) == HAL_SUCCESS);

    hal_rpmsg_deinit();
}

static void task_stats(void *param) {
    for (size_t i = 0;; ++i) {
        hal_log_info("");
        stats_print();
        stats_reset();
        hal_log_info("din: 0x%02lx", (uint32_t)DIO.in);
        hal_log_info("dout: 0x%01lx", (uint32_t)DIO.out);
        vTaskDelay(1000);
    }
}

void initialize_wf_buffers() {
    DAC.waiting_for_data = false;
    DAC.was_set = false;
    DAC.queue = xStreamBufferCreate(DAC_WF_BUFF_SIZE * sizeof(int32_t), 0);
    if (DAC.queue == NULL) {
        hal_log_error("Can't initialize buffer for output waveform");
        hal_panic();
    }

    for (size_t i = 0; i < SKIFIO_ADC_CHANNEL_COUNT; ++i) {
        ADCS[i].queue = xStreamBufferCreate(ADC_WF_BUFF_SIZE * sizeof(int32_t), 0);
        if (ADCS[i].queue == NULL) {
            hal_log_error("Can't initialize buffer for input waveform");
            hal_panic();
        }
    }
}

int main(void) {
    /* Initialize standard SDK demo application pins */
    /* M7 has its local cache and enabled by default,
     * need to set smart subsystems (0x28000000 ~ 0x3FFFFFFF)
     * non-cacheable before accessing this address region */
    BOARD_InitMemory();

    /* Board specific RDC settings */
    BOARD_RdcInit();

    BOARD_BootClockRUN();

    hal_io_uart_init(3);

    copyResourceTable();

#ifdef MCMGR_USED
    /* Initialize MCMGR before calling its API */
    (void)MCMGR_Init();
#endif
    hal_print("\n\r\n\r");
    hal_log_info("** Board started **");

    /*
    hal_log_info("Checking busy wait ...");
    TickType_t meas_start = xTaskGetTickCount();
    hal_busy_wait_ns(1000000000ll);
    hal_log_info("ms per 1e9 busy loop ns: %ld", xTaskGetTickCount() - meas_start);
    */

    initialize_wf_buffers();
    RPMSG_SEND_SEM = xSemaphoreCreateBinary();
    hal_assert(RPMSG_SEND_SEM != NULL);

    hal_log_info("Create statistics task");
    xTaskCreate(task_stats, "Statistics task", TASK_STACK_SIZE, NULL, STATS_TASK_PRIORITY, NULL);

    hal_log_info("Create RPMsg tasks");
    xTaskCreate(task_rpmsg_send, "RPMsg send task", TASK_STACK_SIZE, NULL, RPMSG_TASK_PRIORITY, NULL);
    xTaskCreate(task_rpmsg_recv, "RPMsg receive task", TASK_STACK_SIZE, NULL, RPMSG_TASK_PRIORITY, NULL);

    hal_log_info("Create SkifIO task");
    xTaskCreate(task_skifio, "SkifIO task", TASK_STACK_SIZE, NULL, SKIFIO_TASK_PRIORITY, NULL);

#ifdef GENERATE_SYNC
    hal_log_info("Create sync generator task");
    xTaskCreate(sync_generator_task, "Sync generator task", TASK_STACK_SIZE, NULL, SYNC_TASK_PRIORITY, NULL);
#endif

    vTaskStartScheduler();

    hal_log_error("End of main()");
    hal_panic();

    return 0;
}
