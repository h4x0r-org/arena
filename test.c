#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "h4x0r/alloc.h"
#include "h4x0r/timing.h"
#include "h4x0r/random.h"

#define H4X0R_ARENA_BYTES  (1 << 24)
#define NUM_CONCATENATIONS 1000
#define NUM_ITERATIONS     10000

const char h4x0r_b64[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789_+";

static void
h4x0r_static_random_string(char *outbuf, uint64_t len)
{
    // This fills a static buffer with a random base 64 encoded string
    // by first filling the string with random bytes, and then
    // replacing that byte with a corresponding character in the
    // base64 character set (above).
    //
    // We make the mapping from random value to index by removing the
    // most significant 2 bits of the byte, leaving us w/ a value from
    // 0 to 63.

    h4x0r_random_bytes(outbuf, len);

    for (uint64_t i = 0; i < len; i++) {
        outbuf[i] = h4x0r_b64[outbuf[i] & 0x3f];
    }

    outbuf[len] = 0;
}

static void
h4x0r_build_composite_string(char *outstring, char **parts, uint8_t *sizes)
{
    char *p = outstring;

    for (size_t i = 0; i < NUM_CONCATENATIONS; i++) {
        memcpy(p, parts[i], sizes[i]);
        p += sizes[i];
    }
}

static uint64_t
h4x0r_sum_ascii(char *combined_string, uint64_t combo_len)
{
    uint64_t sum = 0;
    char    *p   = combined_string;

    for (size_t i = 0; i < combo_len; i++) {
        sum += *p++;
    }

    return sum;
}

uint64_t
stdlib_test(void)
{
    char    *parts[NUM_CONCATENATIONS];
    uint8_t  sizes[NUM_CONCATENATIONS];
    uint64_t sum       = 0;
    uint64_t total_len = 0;

    for (size_t i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t combo_len = 0;

        for (size_t j = 0; j < NUM_CONCATENATIONS; j++) {
            uint64_t rand = h4x0r_random_u8();
            sizes[j]      = rand;
            combo_len     = combo_len + rand;
            parts[j]      = calloc(1, sizes[j] + 1);

            h4x0r_static_random_string(parts[j], rand);
        }
        char *combo = malloc(combo_len + 1);
        h4x0r_build_composite_string(combo, parts, sizes);

        for (size_t j = 0; j < NUM_CONCATENATIONS; j++) {
            free(parts[j]);
        }

        total_len += combo_len;
        sum += h4x0r_sum_ascii(combo, combo_len);
        free(combo);
    }

    return sum / total_len;
}

uint64_t
arena_test(void)
{
    h4x0r_arena_t arena;
    h4x0r_initialize_arena(&arena, H4X0R_ARENA_BYTES);

    char    *parts[NUM_CONCATENATIONS];
    uint8_t  sizes[NUM_CONCATENATIONS];
    uint64_t sum       = 0;
    uint64_t total_len = 0;

    for (size_t i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t combo_len = 0;

        for (size_t j = 0; j < NUM_CONCATENATIONS; j++) {
            uint64_t rand = h4x0r_random_u8();
            sizes[j]      = rand;
            combo_len     = combo_len + rand;
            parts[j]      = h4x0r_arena_alloc(&arena, rand + 1);

            h4x0r_static_random_string(parts[j], rand);
        }

        char *combo = h4x0r_arena_alloc(&arena, combo_len + 1);
        h4x0r_build_composite_string(combo, parts, sizes);

        total_len += combo_len;
        sum += h4x0r_sum_ascii(combo, combo_len);
    }

    h4x0r_arena_delete(&arena);

    return sum / total_len;
}

int
main(int argc, char *argv[], char *envp[])
{
    struct timespec start;
    struct timespec stdlib_end;
    struct timespec arena_end;
    struct timespec stdlib_total_time;
    struct timespec arena_total_time;
    uint64_t        stdlib_avg;
    uint64_t        arena_avg;

    h4x0r_save_timestamp(&start);
    stdlib_avg = stdlib_test();
    h4x0r_save_timestamp(&stdlib_end);
    arena_avg = arena_test();
    h4x0r_save_timestamp(&arena_end);

    h4x0r_timespec_diff(&start, &stdlib_end, &stdlib_total_time);
    h4x0r_timespec_diff(&stdlib_end, &arena_end, &arena_total_time);

    printf("stdlib: Avg = %llu in %ld sec, %ld nsec\n",
           (long long)stdlib_avg,
           (long)stdlib_total_time.tv_sec,
           (long)stdlib_total_time.tv_nsec);
    printf("arena: Avg = %llu in %ld sec, %ld nsec\n",
           (long long)arena_avg,
           (long)arena_total_time.tv_sec,
           (long)arena_total_time.tv_nsec);

    return 0;
}
