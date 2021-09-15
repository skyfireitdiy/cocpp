#pragma once

#include "co_default_ctx_factory.h"
#include "co_default_env_factory.h"
#include "co_default_stack_factory.h"
#include "co_define.h"
#include "co_manager.h"
#include "co_singleton.h"

#include <condition_variable>
#include <future>
#include <list>
#include <mutex>

class co_default_manager : public co_manager,
                           public co_singleton<co_default_manager>
{
private:
    std::list<co_env*> env_list__;
    std::recursive_mutex mu_env_list__;

    std::atomic<bool> need_clean__ = { false };

    std::list<std::future<void>> background_task__;
    std::list<co_env*>           expired_env__;
    std::mutex                   mu_expired_env__;
    std::condition_variable      cond_expired_env__;

    std::atomic<int> exist_env_count__ = 0;

    co_env_factory*   env_factory__   = co_default_env_factory::instance();
    co_ctx_factory*   ctx_factory__   = co_default_ctx_factory::instance();
    co_stack_factory* stack_factory__ = co_default_stack_factory::instance();

    size_t default_shared_stack_size__ = CO_DEFAULT_STACK_SIZE;

    co_env* create_env__();
    bool    can_schedule_ctx__(co_env* env) const;

    void clean_env_routine__();

    co_default_manager();

public:
    co_env*           get_best_env() override;
    void              set_env_shared_stack_size(size_t size) override;
    void              set_env_factory(co_env_factory* env_factory) override;
    void              set_ctx_factory(co_ctx_factory* ctx_factory) override;
    void              set_stack_factory(co_stack_factory* stack_factory) override;
    co_env_factory*   env_factory() override;
    co_ctx_factory*   ctx_factory() override;
    co_stack_factory* stack_factory() override;
    void              remove_env(co_env* env) override;
    void              create_env_from_this_thread() override;
    co_env*           current_env() override;
    void              clean_up() override;

    friend class co_singleton<co_default_manager>;
};