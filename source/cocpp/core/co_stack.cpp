#include "cocpp/core/co_stack.h"

#include <cassert>
#include <cstdlib>
#include <sstream>

using namespace std;
CO_NAMESPACE_BEGIN

co_stack::co_stack(co_byte *ptr, size_t stack_size)
    : stack__(ptr)
    , size__(stack_size)
{
}

size_t co_stack::stack_size() const
{
    return size__;
}

co_byte *co_stack::stack() const
{
    return stack__;
}

co_byte *co_stack::stack_top() const
{
    return stack__ + size__;
}

string co_stack::stack_info() const
{
    stringstream ss;
    ss << "  size: " << stack_size() << endl;
    ss << "  stack: 0x" << hex << (void *)stack() << dec << endl;
    ss << "  stack top: 0x" << hex << (void *)stack_top() << dec
       << endl;

    return ss.str();
}

CO_NAMESPACE_END
