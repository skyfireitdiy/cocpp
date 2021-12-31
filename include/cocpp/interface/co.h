_Pragma("once");

#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_return_value.h"
#include "cocpp/utils/co_any.h"
#include "cocpp/utils/co_noncopyable.h"

#include <chrono>
#include <functional>
#include <initializer_list>
#include <optional>
#include <thread>
#include <type_traits>

CO_NAMESPACE_BEGIN

class co_manager;
class co_ctx;

template <typename T>
concept CoIsVoid = std::is_same_v<T, void>;

template <typename T>
concept CoIsNotVoid = !std::is_same_v<T, void>;

class co final : private co_noncopyable
{
private:
    co_ctx*                   ctx__;                                // 当前协程的上下文
    inline static co_manager* manager__ = co_manager::instance();   // 协程管理器
    template <typename Func, typename... Args>                      //
    void init__(co_ctx_config config, Func&& func, Args&&... args); // 初始化协程

public:
    // 静态函数
    CoMemberMethodProxyStatic(manager__, set_if_free_mem_callback);
    CoMemberMethodProxyStatic(manager__, create_env);
    CoMemberMethodProxyStatic(manager__, current_env);
    CoMemberMethodProxyStatic(manager__, set_env_shared_stack_size);
    CoMemberMethodProxyStatic(manager__, set_base_schedule_thread_count);
    CoMemberMethodProxyStatic(manager__, set_max_schedule_thread_count);
    CoMemberMethodProxyStatic(manager__, set_timer_tick_duration);
    CoMemberMethodProxyStatic(manager__, timing_duration);
    CoMemberMethodProxyStatic(manager__, create_ctx);
    CoMemberMethodProxyStatic(manager__, destroy_ctx);
    CoMemberMethodProxyStatic(manager__, create_stack);
    CoMemberMethodProxyStatic(manager__, destroy_stack);

    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), workload, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), state, current_env_);
    CoMemberMethodProxyStatic((manager__->current_env()), schedule_in_this_thread);
    CoMemberMethodProxyStatic((manager__->current_env()), current_ctx);
    CoMemberMethodProxyStatic((manager__->current_env()), can_auto_destroy);

    // 成员函数
    CoMemberMethodProxyWithPrefix(ctx__, state, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, config, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, env, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, can_destroy, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, can_move, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, set_priority, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, priority, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, can_schedule, ctx_);
    std::string name() const;
    co_id       id() const;

    // ctx事件
    CoMemberMethodProxyWithPrefix(ctx__, finished, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, priority_changed, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, state_changed, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, env_set, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, locked_destroy, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, unlocked_destroy, ctx_);
    CoMemberMethodProxyWithPrefix(ctx__, stack_set, ctx_);

    // env事件
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), task_finished, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), ctx_added, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), wait_ctx_timeout, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), wait_ctx_finished, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), state_changed, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), switched_to, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), ctx_removed, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), schedule_stopped, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), schedule_started, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), idle_waited, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), idle_waked, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), all_ctx_removed, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), scheduled_flag_reset, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), schedule_locked, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), schedule_unlocked, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), ctx_taken, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), ctx_initted, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), shared_stack_saved, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), shared_stack_restored, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), all_moveable_ctx_taken, current_env_);
    CoMemberMethodProxyStaticWithPrefix((manager__->current_env()), this_thread_converted_to_schedule_thread, current_env_);

    // manager 事件
    CoMemberMethodProxyStaticWithPrefix(manager__, best_env_got, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, env_created, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, env_shared_stack_size_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, background_task_created, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, env_removed, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, env_from_this_thread_created, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, clean_up_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, env_routine_cleaned, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, base_thread_count_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, max_thread_count_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, ctx_redistributed, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, redundant_env_destroyed, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, timing_routine_finished, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, timing_duration_set, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, all_factory_destroyed, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, background_task_finished, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, ctx_created, manager_);
    CoMemberMethodProxyStaticWithPrefix(manager__, timing_routine_timout, manager_);

    template <typename Func, typename... Args>
    co(Func&& func, Args&&... args); // 构造一个协程，参数为可调用对象与参数列表，如：co c(add, 1, 2);
    template <typename Func, typename... Args>
    co(std::initializer_list<std::function<void(co_ctx_config&)>> opts, Func&& func, Args&&... args); // 使用配置构造一个协程
    template <CoIsNotVoid Ret>
    Ret wait(); // 等待协程执行完毕，返回协程的返回值
    template <CoIsVoid Ret>
    Ret wait(); // 等待协程执行完毕，返回协程的返回值
    template <class Rep, class Period>
    std::optional<co_return_value> wait(const std::chrono::duration<Rep, Period>& wait_duration); // 等待协程执行完毕，返回协程的返回值
    void                           detach();                                                      // 协程分离，协程结束后自动回收
    ~co();
};

#define CoLocal(name, type) []() -> type& {                      \
    return cocpp::co::current_ctx()->local_storage<type>(#name); \
}()

///// 模板实现

template <typename Func, typename... Args>
void co::init__(co_ctx_config config, Func&& func, Args&&... args)
{
    std::function<void(co_any&)> entry;
    if constexpr (std::is_same_v<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>, void>)
    {
        entry = [... args = std::forward<Args>(args), func = std::forward<Func>(func)](co_any& ret) mutable {
            std::forward<Func>(func)(std::forward<Args>(args)...);
        };
    }
    else
    {
        entry = [... args = std::forward<Args>(args), func = std::forward<Func>(func)](co_any& ret) mutable {
            ret = std::forward<Func>(func)(std::forward<Args>(args)...);
        };
    }

    ctx__ = manager__->create_and_schedule_ctx(config, entry, true);
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

template <CoIsNotVoid Ret>
Ret co::wait()
{
    return manager__->current_env()->wait_ctx(ctx__);
}

template <CoIsVoid Ret>
Ret co::wait()
{
    manager__->current_env()->wait_ctx(ctx__);
}

template <class Rep, class Period>
std::optional<co_return_value> co::wait(const std::chrono::duration<Rep, Period>& wait_duration)
{
    return manager__->current_env()->wait_ctx(ctx__, std::chrono::duration_cast<std::chrono::nanoseconds>(wait_duration));
}

CO_NAMESPACE_END