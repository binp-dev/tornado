#include <stdio.h>
#include <stdint.h>

extern "C" {

typedef struct {
    uint8_t bytes[3];
} uint24_t;

};

static_assert(sizeof(uint24_t) == 3);
static_assert(alignof(uint24_t) == 1);

extern "C" {

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t a;
    uint16_t b;
    uint32_t c;
    uint24_t d;
} Packed;

};

static_assert(sizeof(Packed) == 10);
static_assert(alignof(Packed) == 1);
