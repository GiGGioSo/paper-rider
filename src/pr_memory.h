#ifndef PR_MEMORY_H
#define PR_MEMORY_H

#include <cstdint>
#include <cstring>
#include <cassert>

struct MemoryPoolFreeNode {
    MemoryPoolFreeNode *next;
};

struct MemoryPool {
    uint8_t *buffer;
    size_t buffer_len;
    size_t chunk_size;

    MemoryPoolFreeNode *head;
};

void pool_init(MemoryPool *p, void *mem_buffer, size_t mem_buffer_len, size_t chunk_size, size_t alignment);
void *pool_alloc(MemoryPool *p);
void pool_free_all(MemoryPool *p);

inline
uint8_t is_power_of_two(uintptr_t p) {
    return (p & (p-1)) == 0;
}

inline
uintptr_t align_forward(uintptr_t ptr, uintptr_t align) {
    assert(is_power_of_two(align));

    uintptr_t p = ptr;
    uintptr_t mod = p & (align-1);
    if (mod != 0) {
        p += align - mod;
    }
    return p;
}

inline
size_t align_forward_size(size_t ptr, size_t align) {
    assert(is_power_of_two((uintptr_t) align));

    size_t p = ptr;
    size_t mod = p & (align-1);
    if (mod != 0) {
        p += align - mod;
    }
    return p;
}

#endif // PR_MEMORY_H
