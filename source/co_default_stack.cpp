#include "co_default_stack.h"

#include <cassert>
#include <cstdlib>

co_default_stack::co_default_stack(size_t stack_size)
    : size__(stack_size)
{
    assert(size__ % sizeof(void*) == 0);
    raw_mem__ = reinterpret_cast<co_byte*>(malloc(size__ + sizeof(void*) - 1));
    stack__   = reinterpret_cast<co_byte*>(reinterpret_cast<unsigned long long>(raw_mem__ + sizeof(void*) - 1) / sizeof(void*) * sizeof(void*));
}

size_t co_default_stack::stack_size() const
{
    return size__;
}

co_byte* co_default_stack::stack() const
{
    return stack__;
}

co_byte* co_default_stack::stack_top() const
{
    return stack__ + size__;
}

co_default_stack::~co_default_stack()
{
    free(raw_mem__);
}
