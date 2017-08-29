#ifndef STACKTOOLS_H
#define STACKTOOLS_H

#include <stdint.h>
#include <stdbool.h>

uint32_t stack_total_size(void *top, void *bottom);
bool stack_valid(void *stack_bottom, uint32_t value);
uint32_t stack_unused_size(void *top, void *bottom, uint32_t valid_value);

#endif
