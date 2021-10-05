#include <crc.h>

#define CRC16 0x8005

/// The algorithm was copy-pasted from StackOverflow:
/// https://stackoverflow.com/questions/10564491/function-to-calculate-a-crc16-checksum
///
/// There are several details you need to 'match up' with for a particular CRC implementation - even using the same polynomial
/// there can be different results because of minor differences in how data bits are handled, using a particular initial value
/// for the CRC (sometimes it's zero, sometimes 0xffff), and/or inverting the bits of the CRC. For example, sometimes one
/// implementation will work from the low order bits of the data bytes up, while sometimes they'll work from the high order bits
/// down.
///
/// Also, you need to 'push out' the last bits of the CRC after you've run all the data bits through.
///
/// Keep in mind that CRC algorithms were designed to be implemented in hardware, so some of how bit ordering is handled may not
/// make so much sense from a software point of view.
///
/// If you want to match the CRC16 with polynomial 0x8005 as shown on the lammertbies.nl CRC calculator page, you need to make
/// the following changes to your CRC function:
///
///     a) run the data bits through the CRC loop starting from the least significant bit instead of from the most significant bit
///     b) push the last 16 bits of the CRC out of the CRC register after you've finished with the input data
///     c) reverse the CRC bits (I'm guessing this bit is a carry over from hardware implementations)
///
uint16_t calculate_crc16(const uint8_t *data, size_t size)
{
    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    /* Sanity check: */
    if(data == NULL)
        return 0;

    while(size > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1; // item a) work from the least significant bits

        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
        }

        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;

    }

    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }

    // item c) reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return crc;
}
