#pragma once

#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_env.h"
#include "co_manager.h"
#include "co_nocopy.h"
#include "co_return_value.h"
#include <chrono>
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

class co final : public co_nocopy
{
private:
    co_ctx*            ctx__;
    static co_manager* manager__;

    template <typename Func, typename... Args>
    void init__(co_ctx_config config, Func&& func, Args&&... args);

public:
    static void set_custom_scheduler_factory(co_scheduler_factory* scheduler_factory); // 应该在所有协程功能使用前调用

    static void convert_to_schedule_thread(); // 将当前线程转换为调度线程（不能在协程上下文调用）

    static co_env* current_env(); // 当前协程env
    static co_ctx* current_ctx(); // 当前协程ctx
    static co_env* create_env();  // 创建env

    co_id       id() const;
    std::string name() const;

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

namespace this_co
{
co_id       id();    // 协程id
std::string name();  // 协程名称
void        yield(); // 主动让出cpu
template <class Rep, class Period>
void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration); // 协程睡眠
};

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
        config.entry = [this, ... args = std::forward<Args>(args), func = std::forward<Func>(func)](std::any& ret) mutable {
            // CO_O_DEBUG("before run");
            ret = std::forward<Func>(func)(std::forward<Args>(args)...);
            // CO_O_DEBUG("after run");
        };
    }

    ctx__ = manager__->create_and_schedule_ctx(config, true);
}

template <class Rep, class Period>
void this_co::sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) // 协程睡眠
{
    auto start = std::chrono::high_resolution_clock::now();
    do
    {
        this_co::yield();
    } while (std::chrono::high_resolution_clock::now() - start < sleep_duration);
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