#pragma once
#include "co_stack.h"
#include "co_define.h"

class co_default_stack : public co_stack
{
    co_byte* raw_mem__;
    co_byte* stack__;
    size_t   size__;

    co_default_stack(size_t stack_size = CO_DEFAULT_STACK_SIZE);

public:
    size_t   stack_size() const override;
    co_byte* stack() const override;
    co_byte* stack_top() const override;
    ~co_default_stack();

    friend class co_default_stack_factory;
};
