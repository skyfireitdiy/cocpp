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
    // 如果没有可用的env，就创建
    if (env == nullptr)
    {
        return create_env__();
    }
    // 运行到此处说明有可用的env但是没有空闲的env
    if (env_list__.size() < base_thread_count__)
    {
        return create_env__();
    }
    return env;
}

bool co_default_manager::can_schedule_ctx__(co_env* env) const
{
    auto state = env->state();
    return !(state == co_env_state::blocked || state == co_env_state::destorying || state == co_env_state::created);
}

co_env* co_default_manager::create_env__()
{
    auto env = env_factory__->create_env(default_shared_stack_size__);
    env->set_manager(this);

    std::lock_guard<std::recursive_mutex> lock(mu_env_list__);
    env_list__.push_back(env);
    ++exist_env_count__;
    return env;
}

void co_default_manager::set_env_shared_stack_size(size_t size)
{
    default_shared_stack_size__ = size;
}

co_default_manager::co_default_manager(co_scheduler_factory* scheduler_factory,
                                       co_stack_factory*     stack_factory,
                                       co_ctx_factory*       ctx_factory,
                                       co_env_factory*       env_factory)
    : scheduler_factory__(scheduler_factory)
    , stack_factory__(stack_factory)
    , ctx_factory__(ctx_factory)
    , env_factory__(env_factory)
{
    scheduler_factory__->set_manager(this);
    stack_factory__->set_manager(this);
    ctx_factory__->set_manager(this);
    env_factory__->set_manager(this);

    background_task__.emplace_back(std::async([this]() {
        clean_env_routine__();
    }));
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

co_scheduler_factory* co_default_manager::scheduler_factory()
{
    return scheduler_factory__;
}

void co_default_manager::remove_env(co_env* env)
{
    {
        std::lock_guard<std::recursive_mutex> lck(mu_env_list__);
        env_list__.remove(env);
    }
    {
        std::lock_guard<std::mutex> lck(mu_expired_env__);
        CO_DEBUG("push to clean up: %p", env);
        expired_env__.push_back(env);
    }
    cond_expired_env__.notify_one();
}

void co_default_manager::create_env_from_this_thread()
{
    std::lock_guard<std::recursive_mutex> lck(mu_env_list__);
    current_env__ = env_factory__->create_env_from_this_thread(default_shared_stack_size__);
    current_env__->set_manager(this);
    std::lock_guard<std::recursive_mutex> lock(mu_env_list__);
    env_list__.push_back(current_env__);
    ++exist_env_count__;
}

co_env* co_default_manager::current_env()
{
    if (current_env__ == nullptr)
    {
        create_env_from_this_thread();
    }
    return current_env__;
}

bool co_default_manager::clean_up() const
{
    return clean_up__;
}

void co_default_manager::set_clean_up()
{
    clean_up__ = true;
    {
        std::lock_guard<std::recursive_mutex> lck(mu_env_list__);
        auto                                  env_list_back = env_list__; // 在下面的清理操作中需要删除list中的元素导致迭代器失效，此处创建一个副本（也可以直接加入过期列表，然后清空env_list__，但是这样表达力会好些）
        for (auto& env : env_list_back)
        {
            if (!env->has_scheduler_thread())
            {
                // 对于没有调度线程的env，无法将自己加入销毁队列，需要由manager__加入
                remove_env(env);
                CO_DEBUG("push to clean up: %p", env);
                continue;
            }
            env->stop_schedule(); // 注意：没有调度线程的env不能调用stop_schedule
        }
    }
    cond_expired_env__.notify_one();
    for (auto& task : background_task__)
    {
        task.wait();
    }
}

void co_default_manager::clean_env_routine__()
{
    while (!clean_up__ || exist_env_count__ != 0)
    {
        std::unique_lock<std::mutex> lck(mu_expired_env__);
        CO_DEBUG("wait to wake up ...");
        cond_expired_env__.wait(lck);
        CO_DEBUG("wake up clean, exist_env_count: %d, expire count: %u", (int)exist_env_count__, expired_env__.size());
        for (auto& p : expired_env__)
        {
            CO_DEBUG("clean up an env: %p", p);
            env_factory__->destroy_env(p);
            --exist_env_count__;
        }
        expired_env__.clear();
    }
    CO_DEBUG("clean up env finished\n");
}

void co_default_manager::set_base_schedule_thread_count(size_t base_thread_count)
{
    if (base_thread_count == 0)
    {
        base_thread_count = 1;
    }
    base_thread_count__ = base_thread_count;
}
void co_default_manager::set_max_schedule_thread_count(size_t max_thread_count)
{
    if (max_thread_count == 0)
    {
        max_thread_count = 1;
    }
    max_thread_count__ = max_thread_count;
}

void co_default_manager::move_ctx_routine__()
{
    auto merge_list = [](std::list<co_ctx*>& target, const std::list<co_ctx*>& src) {
        target.insert(target.end(), src.begin(), src.end());
    };

    while (!clean_up__)
    {
        std::this_thread::sleep_for(check_duration__);
        {
            std::lock_guard<std::recursive_mutex> lock(mu_env_list__);
            std::list<co_ctx*>                    moved_ctx_list;        // 需要被移动的ctx
            std::list<co_env*>                    blocked_env_list;      // 阻塞的env列表
            std::list<co_env*>                    can_schedule_env_list; // 可调度的env列表
            for (auto& env : env_list__)
            {
                // 如果检测到某个env被阻塞了，先锁定对应env的调度，防止在操作的时候发生调度，然后收集可转移的ctx
                if (is_blocked__(env))
                {
                    env->lock_schedule();
                    env->set_state(co_env_state::blocked); // 设置阻塞状态，后续的add_ctx不会将ctx加入到此env
                    blocked_env_list.push_back(env);
                    auto tmp_moveable_ctx = env->moveable_ctx_list();
                    merge_list(moved_ctx_list, tmp_moveable_ctx); // 将阻塞的env中可移动的ctx收集起来
                    for (auto& ctx : tmp_moveable_ctx)            // 将收集到的可转移的ctx从阻塞的env中取出
                    {
                        env->take_ctx(ctx);
                    }
                }
                else
                {
                    if (can_schedule_ctx__(env)) // 记录可以用来调度的env
                    {
                        can_schedule_env_list.push_back(env);
                    }
                }
                env->reset_scheduled();
            }
            // todo 重新分配
        }
    }
}

bool co_default_manager::is_blocked__(co_env* env) const
{
    auto state = env->state();
    return state != co_env_state::idle && state != co_env_state::created && !env->scheduled();
}