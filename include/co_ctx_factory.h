#pragma once

#include "co_ctx_config.h"
#include "co_singleton.h"

class co_ctx;
class co_stack_factory;

// ctx工厂
class co_ctx_factory
{
public:
    virtual co_ctx* create_ctx(const co_ctx_config& config)            = 0; // 根据配置创建ctx
    virtual void    destroy_ctx(co_ctx* ctx)                           = 0; // 销毁ctx
    virtual void    set_stack_factory(co_stack_factory* stack_factory) = 0; // 设置stack工厂，将用来为ctx创建栈空间
    virtual ~co_ctx_factory() {}
};