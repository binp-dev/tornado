#include <hal/io.h>

size_t __ustd_io_buffer_size = __USTD_IO_BUFFER_SIZE;

char __ustd_io_buffer[__USTD_IO_BUFFER_SIZE + 1];

void __ustd_print_buffer() {
    __ustd_io_buffer[__USTD_IO_BUFFER_SIZE] = '\0';
    PRINTF("%s", __ustd_io_buffer);
}
