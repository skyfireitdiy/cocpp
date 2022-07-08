_Pragma("once");

#include "cocpp/utils/co_singleton.h"

CO_NAMESPACE_BEGIN

class co_env;
class co_ctx;

class co_env_factory final
{
private:
    static co_ctx* create_idle_ctx__();

public:
    static co_env* create_env(size_t stack_size);
    static co_env* create_env_from_this_thread(size_t stack_size);
    static void    destroy_env(co_env* env);
};

CO_NAMESPACE_END