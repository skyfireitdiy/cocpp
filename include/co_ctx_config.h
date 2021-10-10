#pragma once
#include "co_define.h"
#include "co_entry.h"
#include "co_stack_factory.h"
#include <any>
#include <functional>

struct co_ctx_config
{
    std::function<void(std::any&)> entry;                               // 协程执行入口，该入口是被上层调用者使用函数对象或者lambda表达式封装的对象，直接调用
    size_t                         stack_size  = CO_DEFAULT_STACK_SIZE; // 栈大小
    std::string                    name        = "__unknown__";         // 协程名称
    int                            priority    = 99;                    // 优先级 (0~99，数字越小，优先级约高)
    bool                           share_stack = false;                 // 共享栈
};

// 以下宏代码生成 with_xxx 的配置选项
#define CO_GEN_CTX_CONFIG_OPTION(type, name)                                \
    inline std::function<void(co_ctx_config&)> with_##name(                 \
        type p##name)                                                       \
    {                                                                       \
        return [p##name](co_ctx_config& config) { config.name = p##name; }; \
    }

// 以下注释的两项配置由实现自动生成，不需要用户配置，所以将配置接口注释掉
// CO_GEN_CTX_CONFIG_OPTION(std::function<void(std::any&)>, entry)
CO_GEN_CTX_CONFIG_OPTION(size_t, stack_size)
CO_GEN_CTX_CONFIG_OPTION(std::string, name)
CO_GEN_CTX_CONFIG_OPTION(int, priority)
CO_GEN_CTX_CONFIG_OPTION(bool, share_stack)
