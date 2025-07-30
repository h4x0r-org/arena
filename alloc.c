#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

#include "h4x0r/alloc.h"
#include "h4x0r/random.h"

#if defined(H4X0R_ALLOC_LOG)
#if !defined(H4X0R_ALLOC_LOG_DEFAULT)
#define H4X0R_ALLOC_LOG_DEFAULT (0)
#endif
#include <stdio.h>

static _Atomic bool alloc_log = (bool)H4X0R_ALLOC_LOG_DEFAULT;

void
h4x0r_alloc_log_on(void)
{
    atomic_store(&alloc_log, true);
}

void
h4x0r_alloc_log_off(void)
{
    atomic_store(&alloc_log, false);
}

bool
h4x0r_alloc_log_status(void)
{
    return atomic_load(&alloc_log);
}

#endif

static _Atomic size_t h4x0r_page_size = 0;
uint64_t              h4x0r_guard_word;

static inline void
h4x0r_guard_init(void)
{
    h4x0r_random_bytes(&h4x0r_guard_word, sizeof(uint64_t));
}

size_t
h4x0r_get_page_size(void)
{
    size_t result = atomic_load(&h4x0r_page_size);

    if (!result) {
        result = getpagesize();
        atomic_store(&h4x0r_page_size, result);
        h4x0r_guard_init();
    }

    return result;
}

static void
h4x0r_add_arena_segment(h4x0r_arena_t *arena, uint64_t request_len)
{
    h4x0r_segment_t *segment;

    // Spin lock.
    while (atomic_fetch_or(&arena->mutex, 1))
        /* No body */;

    // Check to see if someone else added a segment. If so,
    // unlock and return.
    char *next = atomic_load(&arena->next_alloc);

    if (next && next + request_len < arena->segment_end) {
        atomic_store(&arena->mutex, 0);
        return;
    }

    // Give ourselves at least a page of overhead.
    uint64_t needed = request_len + atomic_load(&h4x0r_page_size)
                    + sizeof(h4x0r_segment_t);
    uint64_t size = 0;

    if (arena->current_segment) {
        size = arena->current_segment->size;
    }

    if (size < needed) {
        size = needed;
    }

    size = h4x0r_pow2_round_up(h4x0r_get_page_size(), size);

    // Here, we're actually getting pages from the OS. We'd expect the
    // map to fail only if we're out of memory.
    segment = mmap(NULL, size, H4X0R_MPROT, H4X0R_MFLAG, -1, 0);

    if (segment == MAP_FAILED) {
        abort(); // out of memory.
    }

    segment->size          = size;
    segment->next_segment  = arena->current_segment;
    arena->next_alloc      = h4x0r_alloc_align(segment->mem);
    arena->segment_end     = ((char *)segment) + size;
    arena->current_segment = segment;

    atomic_store(&arena->mutex, 0);
}

void
h4x0r_initialize_arena(h4x0r_arena_t *arena, uint64_t size)
{
    atomic_store(&arena->next_alloc, NULL);
    atomic_store(&arena->mutex, 0);
    arena->segment_end     = NULL;
    arena->current_segment = NULL;
    arena->heap_allocated  = false;

    h4x0r_add_arena_segment(arena, size);
}

h4x0r_arena_t *
h4x0r_new_arena(uint64_t user_size)
{
    uint64_t       hdr_size = h4x0r_pow2_round_up(h4x0r_get_page_size(),
                                            sizeof(h4x0r_arena_t));
    h4x0r_arena_t *result   = mmap(NULL,
                                 hdr_size,
                                 H4X0R_MPROT,
                                 H4X0R_MFLAG,
                                 -1,
                                 0);

    if (result == MAP_FAILED) {
        abort();
    }

    h4x0r_initialize_arena(result, user_size);
    result->heap_allocated = true;

    return result;
}

void *
_h4x0r_arena_alloc(h4x0r_arena_t *arena,
                   uint64_t       request,
                   char          *file,
                   uint32_t       line)
{
    uint32_t overhead = h4x0r_pow2_round_up(H4X0R_ALIGN,
                                            sizeof(h4x0r_alloc_info_t));

    request = request + overhead;
    request = h4x0r_pow2_round_up(H4X0R_ALIGN, request);

    char *found_value;
    char *desired_value;

    do {
        found_value   = atomic_load(&arena->next_alloc);
        desired_value = found_value + request;

        if (desired_value > arena->segment_end) {
            h4x0r_add_arena_segment(arena, request);
            continue;
        }
    } while (!atomic_compare_exchange_strong(&arena->next_alloc,
                                             &found_value,
                                             desired_value));

    // `found_value` now holds the start of our allocation chunk.
    // The metadata comes first, then we return the address `overhead`
    // bytes after.
    //
    // Remember, while we are returning a void *, we are keeping it `char *`
    // to get byte math.

    h4x0r_alloc_info_t *info   = (h4x0r_alloc_info_t *)found_value;
    char               *result = found_value + overhead;

    *info = (h4x0r_alloc_info_t){
        .guard       = h4x0r_guard_word,
        .file_name   = file,
        .line_number = line,
        .alloc_len   = request,
    };

    h4x0r_alloc_log("%s:%d: alloc %llu bytes @%p (hdr @%p)\n",
                    file,
                    line,
                    request,
                    result,
                    info);

    return result;
}

void
h4x0r_arena_delete(h4x0r_arena_t *arena)
{
    h4x0r_segment_t *segment = arena->current_segment;
    h4x0r_segment_t *next;

    while (segment) {
        next = segment->next_segment;
        munmap(segment, segment->size);
        segment = next;
    }

    if (arena->heap_allocated) {
        uint64_t hdr_size = h4x0r_pow2_round_up(h4x0r_get_page_size(),
                                                sizeof(h4x0r_arena_t));

        munmap(arena, hdr_size);
    }
}
