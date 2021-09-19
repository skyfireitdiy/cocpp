#pragma once

#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_env.h"
#include "co_manager.h"
#include "co_ret.h"
#include <chrono>
#include <initializer_list>
#include <optional>
#include <thread>
#include <type_traits>

class co_manager;
class co_ctx;

template <typename T>
concept co_is_void = std::is_same_v<T, void>;

template <typename T>
concept co_not_void = !std::is_same_v<T, void>;

class co
{
private:
    co_ctx*            ctx__;
    static co_manager* manager__;

    template <typename Func, typename... Args>
    void init__(co_ctx_config config, Func&& func, Args&&... args)
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
                CO_DEBUG("before run");
                ret = std::forward<Func>(func)(std::forward<Args>(args)...);
                CO_DEBUG("after run");
            };
        }

        ctx__ = co::manager__->ctx_factory()->create_ctx(config);
        ctx__->set_flag(CO_CTX_FLAG_HANDLE_BY_CO); // 被co对象持有
        auto env = co::manager__->get_best_env();
        env->add_ctx(ctx__);
    }

public:
    class co_env_destoryer
    {
    public:
        ~co_env_destoryer();
    };

    static void        init_co(co_manager* manager); // 应该在所有协程功能使用前调用，传入manager对象
    static void        uninit_co();                  // 应该在所有协程功能使用完毕后调用，用于清理资源
    static void        schedule_switch();            // 主动让出cpu
    static co_id       id();                         // 协程id
    static std::string name();                       // 协程名称
    static void        convert_to_schedule_thread(); // 将当前线程转换为调度线程（不能在协程上下文调用）
    template <class Rep, class Period>
    static void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) // 协程睡眠
    {
        auto start = std::chrono::steady_clock::now();
        do
        {
            schedule_switch();
        } while (std::chrono::steady_clock::now() - start < sleep_duration);
    }

    // 构造一个协程，自动开始调度，参数为可调用对象与参数列表，如：co c(add, 1, 2);
    template <typename Func, typename... Args>
    co(Func&& func, Args&&... args)
    {
        init__(co_ctx_config {}, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    co(std::initializer_list<std::function<void(co_ctx_config&)>> opts, Func&& func, Args&&... args)
    {
        co_ctx_config config;
        for (auto& cb : opts)
        {
            cb(config);
        }
        init__(config, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    // 等待协程执行结束返回
    template <co_not_void Ret>
    Ret wait()
    {
        // CO_DEBUG("start wait");
        return manager__->current_env()->wait_ctx(ctx__);
    }

    template <co_is_void Ret>
    Ret wait()
    {
        // CO_DEBUG("start wait");
        manager__->current_env()->wait_ctx(ctx__);
    }

    // 等待指定时间
    template <class Rep, class Period>
    std::optional<co_ret> wait(const std::chrono::duration<Rep, Period>& wait_duration)
    {
        return manager__->current_env()->wait_ctx(ctx__, std::chrono::duration_cast<std::chrono::nanoseconds>(wait_duration));
    }

    // 分离协程，之后此协程就不再受到co对象的管理了
    void detach();

    ~co();

    friend class co_env_destoryer;
};
