#pragma once
#include "co_define.h"
#include "co_nocopy.h"
#include "co_type.h"

class co_stack final : public co_nocopy
{
    co_byte* raw_mem__;
    co_byte* stack__;
    size_t   size__;

    co_stack(size_t stack_size = CO_DEFAULT_STACK_SIZE);

public:
    size_t   stack_size() const;
    co_byte* stack() const;
    co_byte* stack_top() const;
    ~co_stack();

    friend class co_stack_factory;
};
