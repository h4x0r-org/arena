#pragma once

// On Linux we are best off using the low-level interface to the
// system PRNG.  On other Unix systems, we fall back to
// `arc4random_buf()` which is widely available.

#if defined(__linux__)
#include <sys/random.h>

static inline void
h4x0r_random_bytes(char *bufptr, size_t len)
{
    if (getrandom(bufptr, len, GRND_NONBLOCK) == -1) {
        abort();
    }
}
#else
#include <stdlib.h>
#define h4x0r_random_bytes(bufptr, len) arc4random_buf(bufptr, len)
#endif

#include <stdint.h>

// clang-format off
#define H4X0R_DECLARE_RAND_FN(type_name, designator) \
                                                     \
static inline type_name                              \
h4x0r_random_##designator(void)                      \
{                                                    \
    type_name result;                                \
                                                     \
    h4x0r_random_bytes(&result, sizeof(type_name));  \
                                                     \
    return result;                                   \
}
// clang-format on

H4X0R_DECLARE_RAND_FN(uint64_t, u64);
H4X0R_DECLARE_RAND_FN(int64_t, i64);
H4X0R_DECLARE_RAND_FN(uint32_t, u32);
H4X0R_DECLARE_RAND_FN(int32_t, i32);
H4X0R_DECLARE_RAND_FN(uint8_t, u8);
H4X0R_DECLARE_RAND_FN(int8_t, i8);
