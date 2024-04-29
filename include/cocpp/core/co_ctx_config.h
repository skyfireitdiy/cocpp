_Pragma("once");
#include "cocpp/core/co_define.h"
#include <functional>
#include <string>

CO_NAMESPACE_BEGIN

class co_env;

struct co_ctx_config final
{
    size_t stack_size {CO_DEFAULT_STACK_SIZE};
    std::string name {"__unknown__"};
    size_t priority {99};
    co_env *bind_env {nullptr};
    bool shared_stack {false};
};

#define CO_GEN_CTX_CONFIG_OPTION(type, name)                              \
    inline std::function<void(co_ctx_config &)> with_##name(type p##name) \
    {                                                                     \
        return [p##name](co_ctx_config &config) {                         \
            config.name = p##name;                                        \
        };                                                                \
    }

#define CO_GEN_CTX_CONFIG_OPTION_HELPER(type, name) \
    CO_GEN_CTX_CONFIG_OPTION(type, name)

CO_GEN_CTX_CONFIG_OPTION_HELPER(size_t, stack_size)
CO_GEN_CTX_CONFIG_OPTION_HELPER(std::string, name)
CO_GEN_CTX_CONFIG_OPTION_HELPER(size_t, priority)
CO_GEN_CTX_CONFIG_OPTION_HELPER(co_env *, bind_env)
CO_GEN_CTX_CONFIG_OPTION_HELPER(bool, shared_stack)

CO_NAMESPACE_END
