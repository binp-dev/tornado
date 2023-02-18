#pragma once

#include <stdint.h>

#include <hal/gpio.h>

void sync_init();
void sync_deinit();

void sync_start(uint32_t period_us);
void sync_stop();
