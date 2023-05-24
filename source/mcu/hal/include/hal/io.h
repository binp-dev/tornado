#pragma once

#include <stdio.h>
#include <stdint.h>

void hal_io_init(uint32_t dev_id);

#define __USTD_IO_BUFFER_SIZE 0x100
extern char __ustd_io_buffer[];

void __ustd_print_buffer();

#include <hal/platform/io.h>