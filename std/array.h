#pragma once

#include <string.h>
#ifdef STD_ARRAY

#include <stddef.h>
#include <stdlib.h>
#include "header.h"

#define arr_push(arr, x)                                                                                               \
                                                                                                                       \
    do {                                                                                                               \
                                                                                                                       \
        if (!arr) {                                                                                                    \
                                                                                                                       \
            Header* header = (Header*)malloc((sizeof(*arr) * INIT_CAPACITY) + sizeof(Header));                         \
            header->count = 0;                                                                                         \
            header->capacity = INIT_CAPACITY;                                                                          \
            arr = (void*)(header + 1);                                                                                 \
        }                                                                                                              \
                                                                                                                       \
        Header* header = (Header*)(arr) - 1;                                                                           \
                                                                                                                       \
        if (header->count >= header->capacity) {                                                                       \
                                                                                                                       \
            header->capacity *= 1.5;                                                                                   \
            header = (Header*)realloc(header, (sizeof(*arr) * header->capacity) + sizeof(Header));                     \
            arr = (void*)(header + 1);                                                                                 \
        }                                                                                                              \
                                                                                                                       \
        (arr)[header->count++] = (x);                                                                                  \
                                                                                                                       \
    } while (0)


#define arr_len(arr) ((arr) ? ((Header*)(arr) - 1)->count : 0)

#define arr_free(arr)                                                                                                  \
    do {                                                                                                               \
        if (arr) free((Header*)(arr) - 1);                                                                             \
    } while (0)


#define foreach(arr, ptr) for (typeof(arr) ptr = (arr); ptr < (arr) + arr_len(arr); ptr++)


#define arr_zeroed(arr)                                                                                                \
                                                                                                                       \
    do {                                                                                                               \
                                                                                                                       \
        if (arr)                                                                                                       \
            memset((arr), 0, sizeof(*(arr)) * arr_len(arr));                                                           \
                                                                                                                       \
    } while (0)


#define arr_init(arr, ...)                                                                                             \
                                                                                                                       \
    do {                                                                                                               \
                                                                                                                       \
        usize count = 0;                                                                                               \
        if (0 __VA_OPT__(+1)) {                                                                                        \
                                                                                                                       \
                typeof(*(arr)) vals[] = {__VA_ARGS__};                                                                 \
                count = sizeof vals / sizeof vals[0];                                                                  \
                                                                                                                       \
            if (arr) {                                                                                                 \
                                                                                                                       \
                arr_free(arr);                                                                                         \
                (arr) = NULL;                                                                                          \
            }                                                                                                          \
                                                                                                                       \
            Header* header = (Header*)malloc(sizeof(Header) + sizeof(*(arr)) * count * 1.5);                           \
            header->count = count;                                                                                     \
            header->capacity = count * 1.5;                                                                            \
            (arr) = (void*)(header + 1);                                                                               \
                                                                                                                       \
            memcpy((arr), vals, sizeof(*(arr)) * count);                                                               \
                                                                                                                       \
        } else {                                                                                                       \
                                                                                                                       \
            if (!arr) {                                                                                                \
                                                                                                                       \
                Header* header = (Header*)malloc(sizeof(Header) + sizeof(*(arr)) * INIT_CAPACITY);                     \
                header->count = 0;                                                                                     \
                header->capacity = INIT_CAPACITY;                                                                      \
                (arr) = (void*)(header + 1);                                                                           \
            }                                                                                                          \
                                                                                                                       \
            arr_zeroed(arr);                                                                                           \
        }                                                                                                              \
                                                                                                                       \
    } while(0)


#define arr_append(arr, ...)                                                                                           \
                                                                                                                       \
    do {                                                                                                               \
                                                                                                                       \
        usize count = 0;                                                                                               \
        if (0 __VA_OPT__(+1)) {                                                                                        \
                                                                                                                       \
            typeof(*(arr)) vals[] = {__VA_ARGS__};                                                                     \
            count = sizeof vals / sizeof vals[0];                                                                      \
                                                                                                                       \
            if (!arr)                                                                                                  \
                arr_init(arr);                                                                                         \
                                                                                                                       \
            Header* header = (Header*)(arr) - 1;                                                                       \
                                                                                                                       \
            if (header->count + count >= header->capacity) {                                                           \
                                                                                                                       \
                header = (Header*)realloc(header, sizeof(Header) + sizeof(*(arr)) * (count + header->capacity * 1.5)); \
                header->capacity = count + header->capacity * 1.5;                                                     \
                (arr) = (void*)(header + 1);                                                                           \
            }                                                                                                          \
                                                                                                                       \
            memcpy((arr) + header->count, vals, sizeof(*(arr)) * count);                                               \
            header->count += count;                                                                                    \
                                                                                                                       \
        } else {                                                                                                       \
                                                                                                                       \
            if (!arr)                                                                                                  \
                arr_init(arr);                                                                                         \
        }                                                                                                              \
                                                                                                                       \
    } while (0)


#endif // STD_ARRAY





