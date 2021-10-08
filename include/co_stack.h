#pragma once

#include "co_nocopy.h"
#include "co_type.h"

#include <cstddef>

// 栈
class co_stack : public co_nocopy
{
public:
    virtual size_t   stack_size() const = 0; // 栈大小
    virtual co_byte* stack() const      = 0; // 栈内存（低地址）
    virtual co_byte* stack_top() const  = 0; // 栈内存（高地址）

    virtual ~co_stack() = default;
};
