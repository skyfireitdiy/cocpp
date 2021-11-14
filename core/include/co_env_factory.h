#pragma once

#include "co_ctx_factory.h"
#include "co_o1_scheduler_factory.h"
#include "co_object_pool.h"
#include "co_singleton.h"
#include "co_stack_factory.h"

CO_NAMESPACE_BEGIN

class co_scheduler;
class co_stack_factory;
class co_scheduler_factory;
class co_env;

class co_env_factory final : public co_singleton<co_env_factory>
{
private:
    co_object_pool<co_env> env_pool__ { MAX_ENV_CACHE_COUNT };

    co_scheduler_factory* scheduler_factory__ { co_o1_scheduler_factory::instance() };
    co_stack_factory*     stack_factory__ { co_stack_factory::instance() };

    co_ctx* create_idle_ctx__();

public:
    co_env* create_env(size_t stack_size);
    co_env* create_env_from_this_thread(size_t stack_size);
    void    destroy_env(co_env* env);
    void    set_scheduler_factory(co_scheduler_factory* scheduler_factory);
    void    free_obj_pool();
};

CO_NAMESPACE_END