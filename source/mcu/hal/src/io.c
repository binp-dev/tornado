#include <hal/io.h>

size_t __hal_io_buffer_size = __HAL_IO_BUFFER_SIZE;

char __hal_io_buffer[__HAL_IO_BUFFER_SIZE + 1];

void __hal_print_buffer() {
    __hal_io_buffer[__HAL_IO_BUFFER_SIZE] = '\0';
    PRINTF("%s", __hal_io_buffer);
}
