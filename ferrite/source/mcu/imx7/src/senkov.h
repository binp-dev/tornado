/*!
 * @brief Драйвер для платы ЦАП-АЦП разработанной Димой Сеньковым.
 */

#pragma once

#include <hal/defs.h>

#define SENKOV_ADC_OUTPUT_CURRENT     0 // (Iвых) дискрет 10мА для 15кА
#define SENKOV_ADC_OUTPUT_VOLTAGE     1 // (Vвых) дискрет 10мV для 40V
#define SENKOV_ADC_DCLINK_VOLTAGE     2 // (V dclink) дискрет 1 мВ
#define SENKOV_ADC_CHANNEL_3          3
#define SENKOV_ADC_SYS_TEMP           4 // (T sys) дискрет 0.1град
#define SENKOV_ADC_CPU_TEMP           5 // (T cpu) дискрет 0.1град
#define SENKOV_ADC_CHANNEL_6          6

typedef struct SenkovControlReg {
    uint8_t ps_enable : 1; // включить источник
    uint8_t : 2;
    uint8_t block_reset : 1; // сброс блокировок
} SenkovControlReg;

hal_retcode senkov_init();
hal_retcode senkov_deinit();

/// запрос измерений канала АЦП
hal_retcode senkov_read_adc(uint32_t index, uint32_t *value);

/// задание напряжения ЦАП. Дискрет 10мВ
hal_retcode senkov_write_dac(uint32_t value);

/// запись в регистр управления
hal_retcode senkov_write_control_reg(SenkovControlReg reg);
/// запись в регистр Set period (период с дискретом 10мкс для тестовых импульсов)
hal_retcode senkov_write_set_period(uint32_t value);
/// запись в регистр Send period (период с дискретом 10мкс для тестовых импульсов)
hal_retcode senkov_write_send_period(uint32_t value);
