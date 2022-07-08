_Pragma("once");

#include "cocpp/core/co_stack_factory.h"
#include "cocpp/utils/co_singleton.h"

CO_NAMESPACE_BEGIN

class co_scheduler;
class co_stack_factory;
class co_scheduler_factory;
class co_env;
class co_ctx;

class co_env_factory final : public co_singleton<co_env_factory>
{
private:
    co_stack_factory* stack_factory__ { co_stack_factory::instance() };
    co_ctx*           create_idle_ctx__();

public:
    co_env* create_env(size_t stack_size);
    co_env* create_env_from_this_thread(size_t stack_size);
    void    destroy_env(co_env* env);
};

CO_NAMESPACE_END