#include "pr_memory.h"

void pool_init(MemoryPool *p, void *mem_buffer, size_t mem_buffer_len,
               size_t chunk_size, size_t alignment) {

    uintptr_t unaligned_start = (uintptr_t) mem_buffer;
    uintptr_t start = align_forward(unaligned_start, (uintptr_t) alignment);

    mem_buffer_len -= (size_t) (start - unaligned_start);

    chunk_size = align_forward_size(chunk_size, alignment);

    // Check to see if the chunk_size is big enough
    assert(chunk_size >= sizeof(MemoryPoolFreeNode) &&
            "Chuck size is too small!");
    // Check that the buffer is bigger than the chunk_size
    assert(mem_buffer_len >= chunk_size &&
            "Chunk size is bigger than the buffer!");

    // TODO: In the gingerbill blog code, the buffer gets set to the
    //          original `mem_buffer`.
    //       Shouldn't it be set to the aligned version of it: `start`?
    p->buffer = (uint8_t *) start; 
    p->buffer_len = mem_buffer_len;
    p->chunk_size = chunk_size;
    p->head = NULL;

    pool_free_all(p);
}

void *pool_alloc(MemoryPool *p) {
    MemoryPoolFreeNode *first_free = p->head;

    if (first_free == NULL) {
        assert(0 && "The pool is out of memory!");
        return NULL;
    }

    p->head = p->head->next;

    return std::memset(first_free, 0, p->chunk_size);
}

void pool_free(MemoryPool *p, void *ptr) {
    void *start = p->buffer;
    void *end = &p->buffer[p->buffer_len];

    if (ptr == NULL) {
        return;
    }

    if (!(start <= ptr && ptr <= end)) {
        assert(0 && "The pointer points outside of the pool buffer!");
        return;
    }

    MemoryPoolFreeNode *new_head = (MemoryPoolFreeNode *) ptr;
    new_head->next = p->head;
    p->head = new_head;
}

void pool_free_all(MemoryPool *p) {
    size_t chunk_count = p->buffer_len / p->chunk_size;

    for (size_t i = 0; i < chunk_count; ++i) {
        void *ptr = &p->buffer[i * p->chunk_size];
        MemoryPoolFreeNode *new_head = (MemoryPoolFreeNode *) ptr;
        new_head->next = p->head;
        p->head = new_head;
    }
}
