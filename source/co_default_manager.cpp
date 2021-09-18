#include "co_default_manager.h"
#include "co_ctx_factory.h"
#include "co_env.h"
#include "co_env_factory.h"
#include <future>

co_env* co_default_manager::get_best_env()
{
    std::lock_guard<std::recursive_mutex> lck(mu_env_list__);
    if (env_list__.empty())
    {
        return create_env__();
    }

    auto env          = env_list__.front();
    auto min_workload = env->workload();
    for (auto& p : env_list__)
    {
        if (p->state() == co_env_state::idle)
        {
            return p;
        }
        if (!can_schedule_ctx__(p))
        {
            continue;
        }
        if (p->workload() < min_workload)
        {
            min_workload = p->workload();
            env          = p;
        }
    }
    return env;
}

bool co_default_manager::can_schedule_ctx__(co_env* env) const
{
    auto state = env->state();
    return !(state == co_env_state::overload || state == co_env_state::destorying || state == co_env_state::created);
}

co_env* co_default_manager::create_env__()
{
    auto env = env_factory__->create_env(default_shared_stack_size__);
    env->set_manager(this);
    env_list__.push_back(env);
    ++exist_env_count__;
    return env;
}

void co_default_manager::set_env_shared_stack_size(size_t size)
{
    default_shared_stack_size__ = size;
}

void co_default_manager::set_env_factory(co_env_factory* env_factory)
{
    env_factory__ = env_factory;
}

void co_default_manager::set_ctx_factory(co_ctx_factory* ctx_factory)
{
    ctx_factory__ = ctx_factory;
}

void co_default_manager::set_stack_factory(co_stack_factory* stack_factory)
{
    stack_factory__ = stack_factory;
}

co_env_factory* co_default_manager::env_factory()
{
    return env_factory__;
}

co_ctx_factory* co_default_manager::ctx_factory()
{
    return ctx_factory__;
}

co_stack_factory* co_default_manager::stack_factory()
{
    return stack_factory__;
}

void co_default_manager::remove_env(co_env* env)
{
    {
        std::lock_guard<std::recursive_mutex> lck(mu_env_list__);
        env_list__.remove(env);
    }
    {
        std::lock_guard<std::mutex> lck(mu_expired_env__);
        expired_env__.push_back(env);
    }
    cond_expired_env__.notify_one();    
    
}

void co_default_manager::create_env_from_this_thread()
{
    std::lock_guard<std::recursive_mutex> lck(mu_env_list__);
    current_env__ = env_factory__->create_env_from_this_thread(default_shared_stack_size__);
    current_env__->set_manager(this);
    ++exist_env_count__;
    env_list__.push_back(current_env__);
}

co_env* co_default_manager::current_env()
{
    if (current_env__ == nullptr)
    {
        create_env_from_this_thread();
    }
    return current_env__;
}

void co_default_manager::clean_up()
{
    {
        std::lock_guard<std::recursive_mutex> lck(mu_env_list__);
        for (auto& env : env_list__)
        {
            env->stop_schedule();
        }
    }
    for (auto& task : background_task__)
    {
        task.wait();
    }
}

void co_default_manager::clean_env_routine__()
{
    do
    {
        std::unique_lock<std::mutex> lck(mu_expired_env__);
        cond_expired_env__.wait(lck, [this] { return !expired_env__.empty(); });
        for (auto& p : expired_env__)
        {
            env_factory__->destroy_env(p);
            --exist_env_count__;
        }
        expired_env__.clear();
    } while (need_clean__ && exist_env_count__ == 0);
}

co_default_manager::co_default_manager()
{
    background_task__.emplace_back(std::async([this]() {
        clean_env_routine__();
    }));
}