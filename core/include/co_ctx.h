_Pragma("once");
#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_event.h"
#include "co_flag_manager.h"
#include "co_local.h"
#include "co_object_pool.h"
#include "co_stack.h"
#include "co_state_manager.h"
#include "co_type.h"

#include <any>
#include <atomic>
#include <bitset>
#include <memory>

CO_NAMESPACE_BEGIN

class co_env;

class co_ctx final : private co_noncopyable,
                     public co_flag_manager<CO_CTX_FLAG_MAX>,
                     public co_state_manager<co_state, co_state::suspended, co_state::finished>
{
    RegCoEvent(finished);
    RegCoEvent(priority_changed, int, int);        // 原优先级
    RegCoEvent(state_changed, co_state, co_state); // 原状态
    RegCoEvent(env_set, co_env*);                  // 设置的env
    RegCoEvent(locked_destroy);
    RegCoEvent(unlocked_destroy);
    RegCoEvent(stack_set, co_stack*); // 设置的stack

private:
    co_ctx_wait_data wait_data__ {};

    co_stack* stack__ { nullptr }; // 当前栈空间

    co_ctx_config       config__ {};                                      // 协程配置
    std::any            ret__;                                            // 协程返回值，会被传递给 config 中的 entry
    co_env*             env__ { nullptr };                                // 协程当前对应的运行环境
    mutable co_spinlock env_lock__ { co_spinlock::lock_type::in_thread }; // 运行环境锁

    int                 priority__ { CO_IDLE_CTX_PRIORITY };                   // 优先级
    mutable co_spinlock priority_lock__ { co_spinlock::lock_type::in_thread }; // 优先级锁

    std::unordered_map<std::string, std::shared_ptr<co_local_base>> locals__; // 协程局部存储

    std::function<void(std::any&)> entry__; // 协程入口函数

#ifdef __GNUC__
#ifdef __x86_64__

    co_byte* regs__[32] {};
#else
#error only supported x86_64
#endif
#endif

    co_ctx(co_stack* stack, const co_ctx_config& config, std::function<void(std::any&)> entry);

public:
    void   set_priority(int priority);
    size_t priority() const;
    bool   can_schedule() const;

    co_stack*                      stack() const;
    co_byte**                      regs();
    const co_ctx_config&           config() const;
    std::any&                      ret_ref();
    void                           set_env(co_env* env);
    co_env*                        env() const;
    bool                           can_destroy() const;
    void                           lock_destroy();
    void                           unlock_destroy();
    void                           set_stack(co_stack* stack);
    bool                           can_move() const;
    std::string                    name() const;
    co_id                          id() const;
    void                           enter_wait_rc_state(int rc_type, void* rc);
    void                           leave_wait_rc_state();
    std::function<void(std::any&)> entry() const;

    static void real_entry(co_ctx* ctx);

    template <typename T>
    T& local(const std::string& name);

    friend class co_object_pool<co_ctx>;
};

// 模板实现

template <typename T>
T& co_ctx::local(const std::string& name)
{
    if (!locals__.contains(name))
    {
        locals__[name] = std::make_shared<co_local<T>>();
    }
    return std::dynamic_pointer_cast<co_local<T>>(locals__[name])->get();
}

CO_NAMESPACE_END