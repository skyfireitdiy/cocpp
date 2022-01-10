#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_env_factory.h"
#include "cocpp/core/co_stack_factory.h"
#include "cocpp/core/co_vos.h"
#include "cocpp/sync/co_spinlock.h"
#include <cassert>
#include <cstddef>
#include <future>
#include <mutex>

CO_NAMESPACE_BEGIN

co_env* co_manager::get_best_env__()
{
    std::scoped_lock lck(env_set__.normal_lock);
    if (env_set__.normal_set.empty())
    {
        return create_env(true);
    }

    co_env* best_env               = nullptr;
    auto    min_workload           = std::numeric_limits<int>::max();
    size_t  can_schedule_env_count = 0;
    for (auto& env : env_set__.normal_set)
    {
        if (env->state() == co_env_state::idle)
        {
            best_env_got().pub(env);
            return env;
        }
        if (!env->can_schedule_ctx())
        {
            continue;
        }
        // 统计可用于调度的env数量
        ++can_schedule_env_count;
        if (env->workload() < min_workload)
        {
            min_workload = env->workload();
            best_env     = env;
        }
    }
    // 如果没有可用的env，就创建
    if (best_env == nullptr)
    {
        auto ret = create_env(true);
        best_env_got().pub(ret);
        return ret;
    }

    // 如果可用于调度的env数量小于基础线程数量，创建一个来调度新的ctx
    if (can_schedule_env_count < env_set__.base_env_count)
    {
        auto ret = create_env(true);
        best_env_got().pub(ret);
        return ret;
    }
    best_env_got().pub(best_env);
    return best_env;
}

void co_manager::subscribe_env_event__(co_env* env)
{
    env->task_finished().sub([this, env]() {
        remove_env__(env);
    });
}

void co_manager::subscribe_ctx_event__(co_ctx* ctx)
{
    ctx->priority_changed().sub([ctx](int old, int new_) {
        ctx->env()->handle_priority_changed(old, ctx);
    });
}

co_env* co_manager::create_env(bool dont_auto_destory)
{
    assert(!clean_up__);
    auto env = factory_set__.env_factory->create_env(default_shared_stack_size__);

    subscribe_env_event__(env);

    if (dont_auto_destory)
    {
        env->set_flag(CO_ENV_FLAG_DONT_AUTO_DESTORY);
    }
    std::scoped_lock lck(env_set__.normal_lock, env_set__.mu_normal_env_count);
    env_set__.normal_set.insert(env);
    ++env_set__.normal_env_count;

    env_created().pub(env);
    return env;
}

void co_manager::set_env_shared_stack_size(size_t size)
{
    default_shared_stack_size__ = size;
    env_shared_stack_size_set().pub(size);
}

void co_manager::create_background_task__()
{
    background_task__.emplace_back(std::async(std::launch::async, [this]() {
        clean_env_routine__();
    }));
    background_task__.emplace_back(std::async(std::launch::async, [this]() {
        timer_routine__();
    }));

    background_task_created().pub();
}

void co_manager::subscribe_manager_event__()
{
    timing_routine_timout().sub([this] {
        // 每两次超时重新分配一次
        static bool double_timeout = false;

        // 强制重新调度
        force_schedule__();

        // 如果是第二次超时
        if (double_timeout)
        {
            // 重新调度
            redistribute_ctx__();
            // 偷取ctx
            steal_ctx_routine__();
            // 销毁多余的env
            destroy_redundant_env__();
            // 释放内存
            free_mem__();
        }
        double_timeout = !double_timeout;
    });
}

void co_manager::free_mem__()
{
    static size_t pass_tick_count = 0;

    pass_tick_count = (pass_tick_count + 1) % TICKS_COUNT_OF_FREE_MEM;
    if (pass_tick_count == 0)
    {
        std::scoped_lock lck(need_free_mem_cb_lock__);
        if (need_free_mem_cb__())
        {
            factory_set__.env_factory->free_obj_pool();
        }
        if (need_free_mem_cb__())
        {
            factory_set__.ctx_factory->free_obj_pool();
        }
        if (need_free_mem_cb__())
        {
            factory_set__.stack_factory->free_obj_pool();
        }
        if (need_free_mem_cb__())
        {
            factory_set__.stack_factory->free_stack_mem_pool();
        }
    }
}

co_manager::co_manager()
{
    subscribe_manager_event__();
    setup_switch_handler();
    create_background_task__();
}

void co_manager::remove_env__(co_env* env)
{
    std::scoped_lock lock(env_set__.normal_lock, env_set__.expired_lock);
    env_set__.normal_set.erase(env);

    std::scoped_lock lck(env_set__.expired_lock);
    env_set__.expired_set.insert(env);
    env_set__.cond_expired_env.notify_one();

    env_removed().pub(env);
}

void co_manager::create_env_from_this_thread__()
{
    std::scoped_lock lck(env_set__.normal_lock, env_set__.mu_normal_env_count);
    current_env__ = factory_set__.env_factory->create_env_from_this_thread(default_shared_stack_size__);

    subscribe_env_event__(current_env__);

    env_set__.normal_set.insert(current_env__);
    ++env_set__.normal_env_count;

    env_from_this_thread_created().pub(current_env__);
}

co_env* co_manager::current_env()
{
    if (current_env__ == nullptr)
    {
        create_env_from_this_thread__();
    }
    return current_env__;
}

void co_manager::set_clean_up__()
{
    std::scoped_lock lock(env_set__.expired_lock, env_set__.normal_lock, clean_up_lock__);
    clean_up__       = true;
    auto backup_data = env_set__.normal_set;
    for (auto& env : backup_data)
    {
        if (env->test_flag(CO_ENV_FLAG_NO_SCHE_THREAD))
        {
            // 对于没有调度线程的env，无法将自己加入销毁队列，需要由manager__加入
            remove_env__(env);
            continue;
        }
        env->stop_schedule(); // 注意：没有调度线程的env不能调用stop_schedule
    }
    env_set__.cond_expired_env.notify_one();

    clean_up_set().pub();
}

void co_manager::clean_env_routine__()
{
    std::unique_lock lck(env_set__.expired_lock);
    std::unique_lock clean_lock(clean_up_lock__);
    while (!clean_up__ || env_set__.normal_env_count != 0)
    {
        clean_lock.unlock();
        env_set__.cond_expired_env.wait(lck);

        clean_lock.lock();
        std::scoped_lock lock(env_set__.mu_normal_env_count);
        for (auto& p : env_set__.expired_set)
        {
            factory_set__.env_factory->destroy_env(p);
            --env_set__.normal_env_count;
        }
        env_set__.expired_set.clear();
    }

    env_routine_cleaned().pub();
}

void co_manager::set_base_schedule_thread_count(size_t base_thread_count)
{
    if (base_thread_count == 0)
    {
        base_thread_count = 1;
    }
    env_set__.base_env_count = base_thread_count;
    base_thread_count_set().pub(env_set__.base_env_count);
}

void co_manager::set_max_schedule_thread_count(size_t max_thread_count)
{
    if (max_thread_count == 0)
    {
        max_thread_count = 1;
    }
    env_set__.max_env_count = max_thread_count;
    max_thread_count_set().pub(env_set__.max_env_count);
}

void co_manager::force_schedule__()
{
    std::scoped_lock lock(env_set__.expired_lock, env_set__.normal_lock, clean_up_lock__);
    if (clean_up__)
    {
        return;
    }
    for (auto& env : env_set__.normal_set)
    {
        // 如果检测到某个env被阻塞了，先锁定对应env的调度，防止在操作的时候发生调度，然后收集可转移的ctx
        if (env->is_blocked())
        {
            // 强行外部调度
            send_switch_from_outside_signal(env);
        }
        env->reset_scheduled_flag();
    }
}

void co_manager::redistribute_ctx__()
{
    std::scoped_lock lock(env_set__.expired_lock, env_set__.normal_lock, clean_up_lock__);
    if (clean_up__)
    {
        return;
    }

    std::list<co_ctx*> moved_ctx_list; // 需要被移动的ctx

    auto merge_list = [](std::list<co_ctx*>& target, const std::list<co_ctx*>& src) {
        target.insert(target.end(), src.begin(), src.end());
    };

    for (auto& env : env_set__.normal_set)
    {
        // 如果检测到某个env被阻塞了，收集可转移的ctx
        if (env->is_blocked())
        {
            // 设置阻塞状态，后续的add_ctx不会将ctx加入到此env
            env->set_state(co_env_state::blocked);

            merge_list(moved_ctx_list, env->take_all_movable_ctx()); // 将阻塞的env中可移动的ctx收集起来
        }
        env->reset_scheduled_flag();
    }
    // 重新选择合适的env进行调度
    for (auto& ctx : moved_ctx_list)
    {
        get_best_env__()->move_ctx_to_here(ctx);
    }

    ctx_redistributed().pub();
}

void co_manager::destroy_redundant_env__()
{
    std::scoped_lock lock(env_set__.normal_lock);
    // 然后删除多余的处于idle状态的env
    size_t               can_schedule_env_count = 0;
    std::vector<co_env*> idle_env_list;
    idle_env_list.reserve(env_set__.normal_set.size());
    for (auto& env : env_set__.normal_set)
    {
        if (env->can_schedule_ctx())
        {
            ++can_schedule_env_count;
        }
        if (env->state() == co_env_state::idle && env->can_auto_destroy()) // 如果状态是空闲，并且可以可以被自动销毁线程选中
        {
            idle_env_list.push_back(env);
        }
    }
    // 超出max_thread_count__，需要销毁env
    if (can_schedule_env_count > env_set__.max_env_count)
    {
        auto should_destroy_count = can_schedule_env_count - env_set__.max_env_count;
        for (size_t i = 0; i < should_destroy_count && i < idle_env_list.size(); ++i)
        {
            idle_env_list[i]->stop_schedule();
        }
    }
    redundant_env_destroyed().pub();
}

void co_manager::timer_routine__()
{
    std::unique_lock lck(clean_up_lock__);
    while (!clean_up__)
    {
        lck.unlock();
        std::this_thread::sleep_for(timing_duration());
        timing_routine_timout().pub();
        lck.lock();
    }
    timing_routine_finished().pub();
}

void co_manager::set_timer_tick_duration(
    const std::chrono::high_resolution_clock::duration& duration)
{
    std::scoped_lock lock(mu_timer_duration__);
    if (duration < std::chrono::milliseconds(DEFAULT_TIMING_TICK_DURATION_IN_MS))
    {
        timer_duration__ = std::chrono::milliseconds(DEFAULT_TIMING_TICK_DURATION_IN_MS);
    }
    else
    {
        timer_duration__ = duration;
    }
    timing_duration_set().pub();
}

const std::chrono::high_resolution_clock::duration& co_manager::timing_duration() const
{
    std::scoped_lock lock(mu_timer_duration__);
    return timer_duration__;
}

co_manager::~co_manager()
{
    set_clean_up__();
    wait_background_task__(); // 此处所有的流程都已经结束了，可以清理一些单例的资源了
    destroy_all_factory__();
}

void co_manager::destroy_all_factory__()
{
    co_stack_factory::destroy_instance();
    co_ctx_factory::destroy_instance();
    co_env_factory::destroy_instance();
    all_factory_destroyed().pub();
}

void co_manager::wait_background_task__()
{
    for (auto& task : background_task__)
    {
        task.wait();
    }
    background_task_finished().pub();
}

co_ctx* co_manager::create_and_schedule_ctx(const co_ctx_config& config, std::function<void(co_any&)> entry, bool lock_destroy)
{
    auto ctx = factory_set__.ctx_factory->create_ctx(config, entry);
    subscribe_ctx_event__(ctx);
    if (lock_destroy)
    {
        ctx->lock_destroy();
    }
    auto bind_env = ctx->config().bind_env;
    if (bind_env != nullptr)
    {
        bind_env->add_ctx(ctx);
    }
    else
    {
        get_best_env__()->add_ctx(ctx);
    }

    ctx_created().pub(ctx);
    return ctx;
}

void co_manager::set_if_free_mem_callback(std::function<bool()> cb)
{
    std::scoped_lock lck(need_free_mem_cb_lock__);
    need_free_mem_cb__ = cb;
}

void co_manager::steal_ctx_routine__()
{
    std::scoped_lock     lock(env_set__.normal_lock);
    std::vector<co_env*> idle_env_list;
    idle_env_list.reserve(env_set__.normal_set.size());
    for (auto& env : env_set__.normal_set)
    {
        if (env->state() == co_env_state::idle)
        {
            idle_env_list.push_back(env);
        }
    }

    auto iter = env_set__.normal_set.begin();
    for (auto& env : idle_env_list)
    {
        for (; iter != env_set__.normal_set.end(); ++iter)
        {
            if ((*iter)->state() == co_env_state::idle)
            {
                break;
            }
            auto ctx = (*iter)->take_one_movable_ctx();
            if (ctx != nullptr)
            {
                env->move_ctx_to_here(ctx);
                break;
            }
        }
    }
}

CO_NAMESPACE_END
