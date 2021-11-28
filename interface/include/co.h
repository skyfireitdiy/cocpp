_Pragma("once");

#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_env.h"
#include "co_manager.h"
#include "co_noncopyable.h"
#include "co_return_value.h"
#include <chrono>
#include <functional>
#include <initializer_list>
#include <optional>
#include <thread>
#include <type_traits>

#include "co_event.h"

CO_NAMESPACE_BEGIN

class co_manager;
class co_ctx;

template <typename T>
concept co_is_void = std::is_same_v<T, void>;

template <typename T>
concept co_not_void = !std::is_same_v<T, void>;

class co final : private co_noncopyable
{
private:
    co_ctx*                   ctx__;
    inline static co_manager* manager__ = co_manager::instance();

    template <typename Func, typename... Args>
    void init__(co_ctx_config config, Func&& func, Args&&... args);

public:
    // 静态函数
    CoMemberMethodProxyStatic(*manager__, set_if_gc_callback);
    CoMemberMethodProxyStatic(*manager__, create_env);
    CoMemberMethodProxyStatic(*manager__, current_env);
    CoMemberMethodProxyStatic(*manager__, set_env_shared_stack_size);
    CoMemberMethodProxyStatic(*manager__, set_base_schedule_thread_count);
    CoMemberMethodProxyStatic(*manager__, set_max_schedule_thread_count);
    CoMemberMethodProxyStatic(*manager__, set_timing_tick_duration);
    CoMemberMethodProxyStatic(*manager__, timing_duration);

    CoMemberMethodProxyStatic(*(co_env_factory::instance()), set_scheduler_factory);

    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), workload, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), state, current_env_);
    CoMemberMethodProxyStatic(*(manager__->current_env()), schedule_in_this_thread);
    CoMemberMethodProxyStatic(*(manager__->current_env()), current_ctx);
    CoMemberMethodProxyStatic(*(manager__->current_env()), scheduler);
    CoMemberMethodProxyStatic(*(manager__->current_env()), can_auto_destroy);

    // 成员函数
    CoMemberMethodProxyWithPrefix(*ctx__, state, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, config, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, env, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, can_destroy, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, can_move, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, set_priority, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, priority, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, can_schedule, ctx_);
    std::string name() const;
    co_id       id() const;

    // ctx事件
    CoMemberMethodProxyWithPrefix(*ctx__, finished, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, priority_changed, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, state_changed, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, env_set, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, locked_destroy, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, unlocked_destroy, ctx_);
    CoMemberMethodProxyWithPrefix(*ctx__, stack_set, ctx_);

    // env事件
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), env_task_finished, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), ctx_added, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), wait_ctx_timeout, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), wait_ctx_finished, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), state_changed, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), switched_to, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), ctx_removed, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), schedule_stopped, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), schedule_started, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), idle_waited, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), idle_waked, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), all_ctx_removed, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), scheduled_flag_reset, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), schedule_locked, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), schedule_unlocked, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), ctx_taken, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), ctx_inited, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), shared_stack_saved, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), shared_stack_restored, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), moveable_ctx_taken, current_env_);
    CoMemberMethodProxyStaticWithPrefix(*(manager__->current_env()), this_thread_converted_to_schedule_thread, current_env_);

    // manager 事件
    CoMemberMethodProxyStaticWithPrefix(*manager__, best_env_got, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, env_created, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, env_shared_stack_size_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, background_task_created, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, env_removed, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, env_from_this_thread_created, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, clean_up_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, env_routine_cleaned, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, base_thread_count_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, max_thread_count_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, ctx_redistributed, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, redundant_env_destroyed, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, timing_routine_finished, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, timing_duration_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, all_factory_destroyed, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, background_task_finished, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, ctx_created, manager_);
    CoMemberMethodProxyStaticWithPrefix(*manager__, timing_routine_timout, manager_);

    // 构造一个协程，自动开始调度，参数为可调用对象与参数列表，如：co c(add, 1, 2);
    template <typename Func, typename... Args>
    co(Func&& func, Args&&... args);

    template <typename Func, typename... Args>
    co(std::initializer_list<std::function<void(co_ctx_config&)>> opts, Func&& func, Args&&... args);

    // 等待协程执行结束返回
    template <co_not_void Ret>
    Ret wait();

    template <co_is_void Ret>
    Ret wait();

    // 等待指定时间
    template <class Rep, class Period>
    std::optional<co_return_value> wait(const std::chrono::duration<Rep, Period>& wait_duration);

    // 分离协程，之后此协程就不再受到co对象的管理了
    void detach();

    ~co();
};

#define CoLocal(name, type) []() -> type& {              \
    return cocpp::co::current_ctx()->local<type>(#name); \
}()

///// 模板实现

template <typename Func, typename... Args>
void co::init__(co_ctx_config config, Func&& func, Args&&... args)
{
    if constexpr (std::is_same_v<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>, void>)
    {
        config.entry = [... args = std::forward<Args>(args), func = std::forward<Func>(func)](std::any& ret) mutable {
            std::forward<Func>(func)(std::forward<Args>(args)...);
        };
    }
    else
    {
        config.entry = [... args = std::forward<Args>(args), func = std::forward<Func>(func)](std::any& ret) mutable {
            // CO_O_DEBUG("before run");
            ret = std::forward<Func>(func)(std::forward<Args>(args)...);
            // CO_O_DEBUG("after run");
        };
    }

    ctx__ = manager__->create_and_schedule_ctx(config, true);
}

template <typename Func, typename... Args>
co::co(Func&& func, Args&&... args)
{
    init__(co_ctx_config {}, std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename Func, typename... Args>
co::co(std::initializer_list<std::function<void(co_ctx_config&)>> opts, Func&& func, Args&&... args)
{
    co_ctx_config config;
    for (auto& cb : opts)
    {
        cb(config);
    }
    init__(config, std::forward<Func>(func), std::forward<Args>(args)...);
}

template <co_not_void Ret>
Ret co::wait()
{
    // CO_O_DEBUG("start wait");
    return manager__->current_env()->wait_ctx(ctx__);
}

template <co_is_void Ret>
Ret co::wait()
{
    // CO_O_DEBUG("start wait");
    manager__->current_env()->wait_ctx(ctx__);
}

template <class Rep, class Period>
std::optional<co_return_value> co::wait(const std::chrono::duration<Rep, Period>& wait_duration)
{
    return manager__->current_env()->wait_ctx(ctx__, std::chrono::duration_cast<std::chrono::nanoseconds>(wait_duration));
}

CO_NAMESPACE_END