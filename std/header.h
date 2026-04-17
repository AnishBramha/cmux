#pragma once

#include <stddef.h>

#define STD_ARRAY_INIT_CAPACITY 256
#define STD_ARRAY_MAX 1024

typedef struct {

    size_t count;
    size_t capacity;

} Header;



