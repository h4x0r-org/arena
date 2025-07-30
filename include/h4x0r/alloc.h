#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define H4X0R_MPROT (PROT_READ | PROT_WRITE)
#define H4X0R_MFLAG (MAP_PRIVATE | MAP_ANON)

#if defined(__GNUC__) && !defined(__clang__)
#define _GNU_SOURCE // necessary on Linux for `getpagesize()`
#endif

#define H4X0R_ALIGN 16

typedef struct h4x0r_segment_t h4x0r_segment_t;

struct h4x0r_segment_t {
    h4x0r_segment_t *next_segment;
    uint64_t         size;
    char             mem[];
};

typedef struct h4x0r_arena_t {
    char            *segment_end;
    _Atomic(char *)  next_alloc;
    h4x0r_segment_t *current_segment;
    _Atomic uint32_t mutex;
    bool             heap_allocated;
} h4x0r_arena_t;

typedef struct h4x0r_alloc_info_t h4x0r_alloc_info_t;

struct h4x0r_alloc_info_t {
    uint64_t guard;
    char    *file_name;
    uint32_t alloc_len;
    uint16_t line_number;
};

// We will only call this where the first argument is a power of 2.
// It will not work if it isn't!
//
// If we want to make this round up in the general case, it should
// be reasonable to replace the below with:
//
// (to_round + power - 1) - (to_round % power)
//
// Honestly, that's not going to be significantly slower.

static inline uint64_t
h4x0r_pow2_round_up(uint64_t power, uint64_t to_round)
{
    uint64_t modulus = power - 1;
    return (to_round + modulus) & ~modulus;
}

// An inlined helper to deal with the casting transparently.
// This is basically a no-op, just makes compile-time easier.

static inline void *
h4x0r_alloc_align(void *addr)
{
    return (void *)h4x0r_pow2_round_up(H4X0R_ALIGN, (uint64_t)addr);
}

extern size_t         h4x0r_get_page_size(void);
extern void           h4x0r_initialize_arena(h4x0r_arena_t *, uint64_t);
extern h4x0r_arena_t *h4x0r_new_arena(uint64_t);
extern void           h4x0r_arena_delete(h4x0r_arena_t *arena);
extern void          *_h4x0r_arena_alloc(h4x0r_arena_t *,
                                         uint64_t,
                                         char *,
                                         uint32_t);

extern uint64_t h4x0r_guard_word;

#define h4x0r_arena_alloc(arena, request) \
    _h4x0r_arena_alloc(arena, request, __FILE__, __LINE__)

#define h4x0r_alloc_instance(arena, type) \
    _h4x0r_arena_alloc(arena, sizeof(type), __FILE__, __LINE__)

#if defined(H4X0R_ALLOC_LOG)

#if !defined(H4X0R_ALLOC_LOG_DEFAULT)
#define H4X0R_ALLOC_LOG_DEFAULT (0)
#endif

extern void h4x0r_alloc_log_on(void);
extern void h4x0r_alloc_log_off(void);
extern bool h4x0r_alloc_log_status(void);

#define h4x0r_alloc_log(...) \
    (h4x0r_alloc_log_status() ? fprintf(stderr, __VA_ARGS__) : 0)

#else
#define h4x0r_alloc_log_on()
#define h4x0r_alloc_log_off()
#define h4x0r_alloc_log_status()
#define h4x0r_alloc_log(...)
#endif
