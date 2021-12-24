_Pragma("once");
#include "co_define.h"
#include "co_stack_factory.h"
#include <any>
#include <functional>

CO_NAMESPACE_BEGIN

class co_env;

struct co_ctx_config
{
    size_t      stack_size { CO_DEFAULT_STACK_SIZE }; // 栈大小
    std::string name { "__unknown__" };               // 协程名称
    size_t      priority { 99 };                      // 优先级 (0~99，数字越小，优先级约高)
    co_env*     bind_env { nullptr };                 // 指定env
    bool        shared_stack { false };               // 共享栈
};

// 以下宏代码生成 with_xxx 的配置选项
#define CO_GEN_CTX_CONFIG_OPTION(type, name)                                \
    inline std::function<void(co_ctx_config&)> with_##name(type p##name)    \
    {                                                                       \
        return [p##name](co_ctx_config& config) { config.name = p##name; }; \
    }

#define CO_GEN_CTX_CONFIG_OPTION_HELPER(type, name) \
    CO_GEN_CTX_CONFIG_OPTION(type, name)

CO_GEN_CTX_CONFIG_OPTION_HELPER(size_t, stack_size)
CO_GEN_CTX_CONFIG_OPTION_HELPER(std::string, name)
CO_GEN_CTX_CONFIG_OPTION_HELPER(size_t, priority)
CO_GEN_CTX_CONFIG_OPTION_HELPER(co_env*, bind_env)
CO_GEN_CTX_CONFIG_OPTION_HELPER(bool, shared_stack)

CO_NAMESPACE_END