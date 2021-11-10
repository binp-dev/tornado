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

#include "skifio.h"

#include "fsl_gpio.h"

#define TASK_STACK_SIZE 256

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

    // GPIO
    gpio_pin_config_t smp_rdy_config = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};
    GPIO_PinInit(GPIO5, 23, &smp_rdy_config);
    gpio_pin_config_t read_rdy_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    GPIO_PinInit(GPIO5, 9, &read_rdy_config);
    hal_log_info("GPIO Initialized");

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

    // Send message back
    hal_assert(HAL_SUCCESS == hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER));
    IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
    mcu_msg->type = IPP_MCU_MSG_DEBUG;
    hal_log_info("Message type: %d", (int)mcu_msg->type);
    const char *message = "Response message";
    mcu_msg->debug.message.len = strlen(message);
    hal_log_info("Message text length: %d", (int)mcu_msg->debug.message.len);
    strcpy(mcu_msg->debug.message.data, message);
    hal_log_info("Whole message size: %d", (int)ipp_mcu_msg_size(mcu_msg));
    hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);

    hal_log_info("SkifIO driver init");
    hal_assert(skifio_init() == HAL_SUCCESS);


    SkifioInput input = {{0}};
    SkifioOutput output = {0};
    for (;;) {
        uint32_t value = 0;
        uint8_t index = 0;
        hal_retcode ret;

        // Receive message
        hal_assert(hal_rpmsg_recv_nocopy(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
        app_msg = (const IppAppMsg *)buffer;
        hal_log_info("Received message: 0x%02x", (int)app_msg->type);

        uint32_t smp_rdy = GPIO_PinRead(GPIO5, 23);
        hal_log_info("SMP_RDY value: %ld", smp_rdy);

        switch (app_msg->type) {
        case IPP_APP_MSG_DAC_SET:
            value = ipp_uint24_load(app_msg->dac_set.value);
            if (value >= 0x10000u) {
                hal_error(0x02, "DAC value is out of bounds (0xffff): 0x%04lx", value);
                continue;
            }
            hal_log_info("Write DAC value: 0x%04lx", value);
            output.dac = value;
            ret = skifio_transfer(&output, &input);
            hal_assert(ret == HAL_SUCCESS || ret == HAL_INVALID_DATA); // Ignore CRC check error
            break;

        case IPP_APP_MSG_ADC_REQ:
            index = app_msg->adc_req.index;
            if (index >= SKIFIO_ADC_CHANNEL_COUNT) {
                hal_error(0x02, "ADC channel index is out of bounds (%i): %i", (int)SKIFIO_ADC_CHANNEL_COUNT, (int)index);
                continue;
            }
            ret = skifio_transfer(&output, &input);
            value = input.adcs[index];
            hal_assert(ret == HAL_SUCCESS || ret == HAL_INVALID_DATA); // Ignore CRC check error
            hal_log_info("Read ADC%d value: 0x%08lx", (int)index, value);

            hal_assert(hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER) == HAL_SUCCESS);
            IppMcuMsg *mcu_msg = (IppMcuMsg *)buffer;
            mcu_msg->type = IPP_MCU_MSG_ADC_VAL;
            mcu_msg->adc_val.index = index;
            mcu_msg->adc_val.value = ipp_uint24_store(value);
            hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_mcu_msg_size(mcu_msg)) == HAL_SUCCESS);

            break;

        default:
            hal_log_error("Wrong message type: %d", (int)app_msg->type);
            hal_panic();
        }
        hal_assert(hal_rpmsg_free_rx_buffer(&channel, buffer) == HAL_SUCCESS);
        GPIO_PinWrite(GPIO5, 9, 1);
        vTaskDelay(1);
        GPIO_PinWrite(GPIO5, 9, 0);
    }

    hal_log_error("End of task_rpmsg()");
    hal_panic();

    hal_assert(skifio_deinit() == HAL_SUCCESS);

    hal_log_error("End of task_rpmsg()");
    hal_panic();

    /* FIXME: Should never reach this point - otherwise virtio hangs */
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

    /* Create tasks. */
    xTaskCreate(
        task_rpmsg, "RPMSG task",
        TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    hal_log_error("End of main()");
    hal_panic();

    return 0;
}
