#include <assert.h>
#include <stdint.h>

struct my0_t {
    uint8_t foo;
    uint32_t bar;
} var0[3];
static_assert(sizeof(var0) == 8 * 3, "Unexpected sizeof");

struct my1_t {
    uint8_t foo;
    uint8_t _padding[3];
    uint32_t bar;
} var1[3];
static_assert(sizeof(var1) == 8 * 3, "Unexpected sizeof");

struct my2_t {
    uint8_t _padding[3];
    uint8_t foo;
    uint32_t bar;
} var2[3];
static_assert(sizeof(var2) == 8 * 3, "Unexpected sizeof");

struct my3_t {
    uint8_t foo;
    uint32_t bar;
} __attribute__((packed));
struct my3_t var3[3];
static_assert(sizeof(var3) == 5 * 3, "Unexpected sizeof");

struct my4_t {
    uint32_t bar;
    uint8_t foo;
} var4[3];
static_assert(sizeof(var4) == 8 * 3, "Unexpected sizeof");

struct my5_t {
    struct my4_t baz;
    uint8_t foo;
} var5[3];
static_assert(sizeof(var5) == 12 * 3, "Unexpected sizeof");

struct my6_t {
    uint8_t foo;
    uint32_t bar;
} __attribute__((aligned(32))) var6[3];
static_assert(sizeof(var6) == 32 * 3, "Unexpected sizeof");

struct my7_t {
    uint8_t foo;
    struct inner {
        uint8_t foo;
    } __attribute__((aligned(32))) bar;
} var7[3];
static_assert(sizeof(var7) == 64 * 3, "Unexpected sizeof");
