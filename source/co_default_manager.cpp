#include "co_default_manager.h"
#include "co_ctx_factory.h"
#include "co_env.h"
#include "co_env_factory.h"
#include <cassert>
#include <cstddef>
#include <future>
#include <mutex>

co_env* co_default_manager::get_best_env()
{
    std::lock_guard<std::recursive_mutex> lck(mu_env_list__);
    if (env_list__.empty())
    {
        return create_env__();
    }

    auto   env                    = env_list__.front();
    auto   min_workload           = env->workload();
    size_t can_schedule_env_count = 0;
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
        // 统计可用于调度的env数量
        ++can_schedule_env_count;
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

    // 如果可用于调度的env数量小于基础线程数量，创建一个来调度新的ctx
    if (can_schedule_env_count < base_thread_count__)
    {
        return create_env__();
    }
    return env;
}

bool co_default_manager::can_schedule_ctx__(co_env* env) const
{
    auto state = env->state();
    return !(state == co_env_state::blocked || state == co_env_state::destorying || !env->has_scheduler_thread());
}

co_env* co_default_manager::create_env__()
{
    assert(!clean_up__);
    auto env = env_factory__->create_env(default_shared_stack_size__);
    env->set_manager(this);

    std::lock_guard<std::recursive_mutex> lock(mu_env_list__);
    env_list__.push_back(env);
    ++exist_env_count__;
    CO_O_DEBUG("create env : %p", env);
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
    background_task__.emplace_back(std::async([this]() {
        timing_routine__();
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
    std::scoped_lock lock(mu_env_list__, mu_clean_up__);
    env_list__.remove(env);
    std::lock_guard<std::recursive_mutex> lck(mu_clean_up__);
    CO_O_DEBUG("push to clean up: %p", env);
    expired_env__.push_back(env);
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
    CO_O_DEBUG("create env from this thread : %p", current_env__);
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
    {
        std::scoped_lock lock(mu_clean_up__, mu_env_list__);
        CO_O_DEBUG("set clean up!!!");
        clean_up__         = true;
        auto env_list_back = env_list__; // 在下面的清理操作中需要删除list中的元素导致迭代器失效，此处创建一个副本（也可以直接加入过期列表，然后清空env_list__，但是这样表达力会好些）
        for (auto& env : env_list_back)
        {
            if (!env->has_scheduler_thread())
            {
                // 对于没有调度线程的env，无法将自己加入销毁队列，需要由manager__加入
                remove_env(env);
                CO_O_DEBUG("push to clean up: %p", env);
                continue;
            }
            CO_O_DEBUG("call stop_schedule on %p", env);
            env->stop_schedule(); // 注意：没有调度线程的env不能调用stop_schedule
        }
        cond_expired_env__.notify_one();
    }
    for (auto& task : background_task__)
    {
        task.wait();
    }
}

void co_default_manager::clean_env_routine__()
{
    std::unique_lock<std::recursive_mutex> lck(mu_clean_up__);
    while (!clean_up__ || exist_env_count__ != 0)
    {
        CO_O_DEBUG("wait to wake up ...");
        cond_expired_env__.wait(lck);
        CO_O_DEBUG("wake up clean, exist_env_count: %d, expire count: %lu", (int)exist_env_count__, expired_env__.size());
        for (auto& p : expired_env__)
        {
            CO_O_DEBUG("clean up an env: %p", p);
            env_factory__->destroy_env(p);
            --exist_env_count__;
        }
        expired_env__.clear();
    }
    CO_O_DEBUG("clean up env finished\n");
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

void co_default_manager::redistribute_ctx__()
{
    // 此处也需要锁定mu_env_list__，上层锁定
    std::lock_guard<std::recursive_mutex> lock(mu_clean_up__);
    if (clean_up__)
    {
        return;
    }

    std::list<co_ctx*> moved_ctx_list; // 需要被移动的ctx

    auto merge_list = [](std::list<co_ctx*>& target, const std::list<co_ctx*>& src) {
        target.insert(target.end(), src.begin(), src.end());
    };

    for (auto& env : env_list__)
    {
        // 如果检测到某个env被阻塞了，先锁定对应env的调度，防止在操作的时候发生调度，然后收集可转移的ctx
        if (is_blocked__(env))
        {
            CO_O_DEBUG("env %p is blocked, redistribute ctx", env);
            env->lock_schedule();
            env->set_state(co_env_state::blocked); // 设置阻塞状态，后续的add_ctx不会将ctx加入到此env
            auto tmp_moveable_ctx = env->moveable_ctx_list();
            merge_list(moved_ctx_list, tmp_moveable_ctx); // 将阻塞的env中可移动的ctx收集起来
            for (auto& ctx : tmp_moveable_ctx)            // 将收集到的可转移的ctx从阻塞的env中取出
            {
                env->take_ctx(ctx);
            }
            env->unlock_schedule(); // 恢复调度
        }
        env->reset_scheduled();
    }
    // 重新选择合适的env进行调度
    for (auto& ctx : moved_ctx_list)
    {
        get_best_env()->add_ctx(ctx);
    }
}

void co_default_manager::destroy_redundant_env__()
{
    std::lock_guard<std::recursive_mutex> lock(mu_env_list__);
    // 然后删除多余的处于idle状态的env
    size_t               can_schedule_env_count = 0;
    std::vector<co_env*> idle_env_list;
    idle_env_list.reserve(env_list__.size());
    for (auto& env : env_list__)
    {
        if (can_schedule_ctx__(env))
        {
            ++can_schedule_env_count;
        }
        if (env->state() == co_env_state::idle && env->can_auto_destroy()) // 如果状态是空闲，并且可以可以被自动销毁线程选中
        {
            idle_env_list.push_back(env);
        }
    }
    // 超出max_thread_count__，需要销毁env
    if (can_schedule_env_count > max_thread_count__)
    {
        auto should_destroy_count = can_schedule_env_count - max_thread_count__;
        for (size_t i = 0; i < should_destroy_count && i < idle_env_list.size(); ++i)
        {
            idle_env_list[i]->stop_schedule();
        }
    }
}

void co_default_manager::timing_routine__()
{
    while (!clean_up__)
    {
        std::this_thread::sleep_for(timing_duration());
        redistribute_ctx__();
        destroy_redundant_env__();
    }
}

bool co_default_manager::is_blocked__(co_env* env) const
{
    auto state = env->state();
    return state != co_env_state::idle && state != co_env_state::created && !env->scheduled();
}

void co_default_manager::set_timing_duration(
    const std::chrono::high_resolution_clock::duration& duration)
{
    std::lock_guard<std::mutex> lock(mu_timing_duration__);
    if (duration < std::chrono::milliseconds(10))
    {
        timing_duration__ = std::chrono::milliseconds(10);
    }
    else
    {
        timing_duration__ = duration;
    }
}

const std::chrono::high_resolution_clock::duration& co_default_manager::timing_duration() const
{
    std::lock_guard<std::mutex> lock(mu_timing_duration__);
    return timing_duration__;
}