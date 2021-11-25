#pragma once
#include "co_define.h"
#include "co_noncopyable.h"
#include "co_object_pool.h"
#include "co_type.h"

CO_NAMESPACE_BEGIN

class co_stack final : private co_noncopyable
{
    co_byte* stack__;
    size_t   size__;

    co_stack(co_byte* ptr, size_t stack_size);

public:
    size_t   stack_size() const;
    co_byte* stack() const;
    co_byte* stack_top() const;

    friend class co_object_pool<co_stack>;
};

CO_NAMESPACE_END