#pragma once
#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_event.h"
#include "co_flag_manager.h"
#include "co_stack.h"
#include "co_type.h"

#include <any>
#include <atomic>
#include <bitset>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_env;

class co_ctx final : public co_nocopy,
                     public co_flag_manager<CO_CTX_FLAG_MAX>
{
    RegCoEvent(co_finished, co_ctx*);

private:
    co_stack*             stack__ { nullptr };           // 当前栈空间
    std::atomic<co_state> state__ { co_state::created }; // 协程状态
    co_ctx_config         config__ {};                   // 协程配置
    std::any              ret__;                         // 协程返回值，会被传递给 config 中的 entry
    co_env*               env__ { nullptr };             // 协程当前对应的运行环境

    std::atomic<int> priority__ { CO_IDLE_CTX_PRIORITY }; // 优先级

#ifdef __GNUC__
#ifdef __x86_64__

    co_byte* regs__[11] {};
#else
#error only supported x86_64
#endif
#endif

    co_ctx(co_stack* stack, const co_ctx_config& config);

public:
    co_stack*            stack() const;
    co_state             state() const;
    co_byte**            regs();
    void                 set_state(co_state state);
    const co_ctx_config& config() const;
    std::any&            ret_ref();
    void                 set_env(co_env* env);
    co_env*              env() const;
    void                 set_priority(int priority);
    int                  priority() const;
    bool                 can_destroy() const;
    void                 lock_destroy();
    void                 unlock_destroy();
    void                 set_stack(co_stack* stack);

    friend void co_entry(co_ctx* ctx);
    friend class co_ctx_factory;
};

CO_NAMESPACE_END