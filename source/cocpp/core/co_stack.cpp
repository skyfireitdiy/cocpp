#include "cocpp/core/co_stack.h"

#include <cassert>
#include <cstdlib>

CO_NAMESPACE_BEGIN

co_stack::co_stack(co_byte* ptr, size_t stack_size)
    : stack__(ptr)
    , size__(stack_size)
{
    // CO_O_DEBUG("malloc raw_mem__ = %p", raw_mem__);
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

CO_NAMESPACE_END