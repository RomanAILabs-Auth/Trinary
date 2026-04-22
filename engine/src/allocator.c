/*
 * allocator.c — 64-byte minimum aligned allocator with OS-backed mappings.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * Strategy:
 *   - Windows: VirtualAlloc (try MEM_LARGE_PAGES, fallback normal commit).
 *   - POSIX:   mmap + madvise(MADV_HUGEPAGE) hint.
 *   - Fallback: heap malloc if OS mapping fails.
 *
 * We place a small header immediately before the returned aligned pointer so
 * trinary_v1_free can correctly release the original base mapping.
 */
#include "trinary/trinary.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <sys/mman.h>
  #include <unistd.h>
#endif

#define TRI_ALLOC_MAGIC 0x545249414C4C4F43ULL /* "TRIALLOC" */

typedef enum tri_alloc_kind {
    TRI_ALLOC_HEAP = 1,
    TRI_ALLOC_OSMAP = 2
} tri_alloc_kind;

typedef struct tri_alloc_hdr {
    uint64_t magic;
    void    *base;
    size_t   map_size;
    uint32_t kind;
    uint32_t reserved;
} tri_alloc_hdr;

static uintptr_t align_up_uintptr(uintptr_t p, size_t a) {
    return (p + (uintptr_t)(a - 1)) & ~(uintptr_t)(a - 1);
}

void *trinary_v1_alloc(size_t bytes, size_t alignment) {
    if (alignment < 64) alignment = 64;
    if ((alignment & (alignment - 1)) != 0) return NULL;
    if (bytes == 0) bytes = 1;

    const size_t overhead = sizeof(tri_alloc_hdr) + alignment - 1;
    if (bytes > (size_t)-1 - overhead) return NULL;
    const size_t total = bytes + overhead;

    void *base = NULL;
    size_t mapped = 0;
    tri_alloc_kind kind = TRI_ALLOC_HEAP;
    const int try_osmap = (bytes >= (128ull << 20)); /* reserve OS mapping for very large arenas */

#if defined(_WIN32)
    if (try_osmap) {
        const SIZE_T req = (SIZE_T)total;
        SIZE_T alloc_sz = req;
        DWORD flags = MEM_RESERVE | MEM_COMMIT;
        SIZE_T lp_min = GetLargePageMinimum();
        if (lp_min > 0 && req >= lp_min) {
            alloc_sz = (req + lp_min - 1) / lp_min * lp_min;
            base = VirtualAlloc(NULL, alloc_sz, flags | MEM_LARGE_PAGES, PAGE_READWRITE);
            if (base) {
                mapped = (size_t)alloc_sz;
                kind = TRI_ALLOC_OSMAP;
            }
        }
        if (!base) {
            alloc_sz = (SIZE_T)((req + 4095u) & ~4095u);
            base = VirtualAlloc(NULL, alloc_sz, flags, PAGE_READWRITE);
            if (base) {
                mapped = (size_t)alloc_sz;
                kind = TRI_ALLOC_OSMAP;
            }
        }
    }
#else
    if (try_osmap) {
        long pg = sysconf(_SC_PAGESIZE);
        size_t page = (pg > 0) ? (size_t)pg : 4096u;
        size_t map_sz = (total + page - 1) / page * page;
        base = mmap(NULL, map_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base != MAP_FAILED) {
            mapped = map_sz;
            kind = TRI_ALLOC_OSMAP;
            #if defined(MADV_HUGEPAGE)
            (void)madvise(base, map_sz, MADV_HUGEPAGE);
            #endif
        } else {
            base = NULL;
        }
    }
#endif

    if (!base) {
        base = malloc(total);
        if (!base) return NULL;
        mapped = total;
        kind = TRI_ALLOC_HEAP;
    }

    uintptr_t p = (uintptr_t)base + sizeof(tri_alloc_hdr);
    uintptr_t aligned = align_up_uintptr(p, alignment);
    tri_alloc_hdr *hdr = (tri_alloc_hdr *)(aligned - sizeof(tri_alloc_hdr));
    hdr->magic = TRI_ALLOC_MAGIC;
    hdr->base = base;
    hdr->map_size = mapped;
    hdr->kind = (uint32_t)kind;
    hdr->reserved = 0;
    return (void *)aligned;
}

void trinary_v1_free(void *ptr) {
    if (!ptr) return;
    tri_alloc_hdr *hdr = (tri_alloc_hdr *)((uintptr_t)ptr - sizeof(tri_alloc_hdr));
    if (hdr->magic != TRI_ALLOC_MAGIC || !hdr->base) {
        /* Defensive fallback for unknown pointers. */
        free(ptr);
        return;
    }

    if (hdr->kind == TRI_ALLOC_OSMAP) {
#if defined(_WIN32)
        (void)VirtualFree(hdr->base, 0, MEM_RELEASE);
#else
        (void)munmap(hdr->base, hdr->map_size);
#endif
    } else {
        free(hdr->base);
    }
}
