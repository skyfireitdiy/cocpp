#pragma once

#include "co_default_ctx_factory.h"
#include "co_default_stack_factory.h"
#include "co_env_factory.h"
#include "co_o1_scheduler_factory.h"
#include "co_singleton.h"

class co_scheduler;
class co_stack_factory;
class co_scheduler_factory;

class co_default_env_factory final : public co_env_factory,
                                     public co_singleton<co_default_env_factory>
{
private:
    co_manager* manager__ = nullptr;
    co_ctx*     create_idle_ctx__();

public:
    co_env* create_env(size_t stack_size) override;
    co_env* create_env_from_this_thread(size_t stack_size) override;
    void    destroy_env(co_env* env) override;
    void    set_manager(co_manager* manager) override;
};