#ifndef PR_COMMON_H
#define PR_COMMON_H

#include "pr_mathy.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#define return_defer(ret) do { result = ret; goto defer; } while(0)

#define UNUSED(expr) (void)(expr)

#define DA_INITIAL_CAPACITY 32

#define da_append(da, item, T)                                            \
do {                                                                      \
    if ((da)->count >= (da)->capacity) {                                  \
        (da)->capacity = ((da)->capacity == 0) ?                          \
            DA_INITIAL_CAPACITY : (da)->capacity*2;                       \
        (da)->items = (T *) realloc((da)->items,                          \
                              (da)->capacity*sizeof(T));                  \
        assert((da)->items != NULL && "Buy more RAM lol");                \
    }                                                                     \
    (da)->items[(da)->count++] = (item);                                  \
} while (0)

#define da_remove(da, index)                                              \
do {                                                                      \
    size_t type_size = sizeof(*(da)->items);                              \
    memmove((uint8_t *)((da)->items) + (index) * type_size,               \
                 (uint8_t *)((da)->items) + ((index)+1) * type_size,      \
                 ((da)->count - (index) - 1) * type_size);                \
    (da)->count--;                                                        \
} while (0)

#define da_clear(da)                                                      \
do {                                                                      \
    if ((da)->items) free((da)->items);                                   \
    (da)->items = NULL;                                                   \
    (da)->count = 0;                                                      \
    (da)->capacity = 0;                                                   \
} while (0)

#define da_swap(da, i, j, T)                                              \
do {                                                                      \
    if (i >= (da)->count || j >= (da)->count) {                           \
        fprintf(stderr,                                                   \
                "[ERROR] Accessing element %zu from dymanic array of length %zu\n",\
                ((i > j) ? i : j),                                        \
                (da)->count);                                             \
    }                                                                     \
    T tmp = (da)->items[i];                                               \
    (da)->items[i] = (da)->items[j];                                      \
    (da)->items[j] = tmp;                                                 \
} while (0)

#define da_last(da) ((da)->items[(da)->count-1])


unsigned char *read_whole_file(const char *path);


#endif // PR_COMMON_H
