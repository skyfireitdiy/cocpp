#include "co_stack.h"

#include <cassert>
#include <cstdlib>

co_stack::co_stack(size_t stack_size)
    : size__(stack_size)
{
    assert(size__ % sizeof(void*) == 0);
    if (size__ == 0)
    {
        raw_mem__ = nullptr;
        stack__   = nullptr;
        return;
    }
    raw_mem__ = reinterpret_cast<co_byte*>(malloc(size__ + sizeof(void*) - 1));
    stack__   = reinterpret_cast<co_byte*>(reinterpret_cast<unsigned long long>(raw_mem__ + sizeof(void*) - 1) / sizeof(void*) * sizeof(void*));
    CO_O_DEBUG("malloc raw_mem__ = %p", raw_mem__);
}

size_t co_stack::stack_size() const
{
    return size__;
}

co_byte* co_stack::stack() const
{
    return stack__;
}

co_byte* co_stack::stack_top() const
{
    return stack__ + size__;
}

co_stack::~co_stack()
{
    CO_O_DEBUG("free raw_mem__ = %p", raw_mem__);
    free(raw_mem__);
}
