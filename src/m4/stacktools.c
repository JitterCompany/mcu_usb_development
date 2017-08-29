#include <string.h>
#include "stacktools.h"

bool stack_valid(void *stack_bottom, uint32_t value)
{
    return (*(uint32_t *)stack_bottom == value);
}

uint32_t stack_unused_size(void *top, void *bottom, uint32_t valid_value)
{
    const uint32_t *ptr = (uint32_t*)bottom;
    while(*ptr == valid_value) {
        ptr++;
        if((void*)ptr >= top) {
            break;
        }
    }
    return (uint32_t) ((uintptr_t)ptr - (uintptr_t)bottom);
}

uint32_t stack_total_size(void *top, void *bottom)
{
    return (uint32_t) ((uintptr_t)top - (uintptr_t)bottom);
}
