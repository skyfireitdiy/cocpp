#pragma once

#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_env.h"
#include "co_manager.h"
#include "co_ret.h"
#include <chrono>
#include <optional>
#include <thread>
#include <type_traits>

class co_manager;
class co_ctx;

class co
{
private:
    co_ctx*            ctx__;
    static co_manager* manager__;

public:
    class co_env_destoryer
    {
    public:
        ~co_env_destoryer();
    };

    static void init_co(co_manager* manager); // 应该在所有协程功能使用前调用，传入manager对象
    static void uninit_co();                  // 应该在所有协程功能使用完毕后调用，用于清理资源
    static void schedule_switch();            // 主动让出cpu

    // 构造一个协程，自动开始调度，参数为可调用对象与参数列表，如：co c(add, 1, 2);
    template <typename Func, typename... Args>
    co(Func&& func, Args&&... args)
    {
        co_ctx_config config;
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

        ctx__    = co::manager__->ctx_factory()->create_ctx(config);
        auto env = co::manager__->get_best_env();
        env->add_ctx(ctx__);
    }

    // 等待协程执行结束返回
    template <typename Ret>
    Ret wait()
    {
        CO_DEBUG("start wait");
        return manager__->current_env()->wait_ctx(ctx__);
    }

    // 对于无返回值函数的特化
    template <>
    void wait()
    {
        manager__->current_env()->wait_ctx(ctx__);
    }

    // 等待指定时间
    std::optional<co_ret> wait(const std::chrono::milliseconds& timeout);

    // 分离协程，之后此协程就不再受到co对象的管理了
    void detach();

    friend class co_env_destoryer;
};
