#pragma once

#include "co_stack.h"

class co_manager;

// stack工厂
class co_stack_factory
{
public:
    virtual co_stack* create_stack(size_t size)        = 0; // 根据大小创建栈空间
    virtual void      destroy_stack(co_stack* stack)   = 0; // 销毁栈空间
    virtual void      set_manager(co_manager* manager) = 0; // 设置manager

    virtual ~co_stack_factory() = default;
};
