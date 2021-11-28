_Pragma("once");
#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_event.h"
#include "co_flag_manager.h"
#include "co_object_pool.h"
#include "co_scheduler.h"
#include "co_stack.h"
#include "co_type.h"

#include <any>
#include <atomic>
#include <bitset>
#include <memory>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_env;

class co_ctx final : private co_noncopyable,
                     public co_flag_manager<CO_CTX_FLAG_MAX>
{
    RegCoEvent(finished);
    RegCoEvent(priority_changed, int, int);        // 原优先级
    RegCoEvent(state_changed, co_state, co_state); // 原状态
    RegCoEvent(env_set, co_env*);                  // 设置的env
    RegCoEvent(locked_destroy);
    RegCoEvent(unlocked_destroy);
    RegCoEvent(stack_set, co_stack*); // 设置的stack

private:
    class inner_local_base
    {
    public:
        virtual ~inner_local_base() = default;
    };

    template <typename T>
    class inner_local : public inner_local_base
    {
    private:
        T data__;

    public:
        T& get()
        {
            return data__;
        }
    };

    struct
    {
        std::mutex mu;
        int        type;
        void*      rc;
    } wait_data__;

    co_stack*          stack__ { nullptr }; // 当前栈空间
    mutable std::mutex lock_state__;
    co_state           state__ { co_state::suspended };     // 协程状态
    co_ctx_config      config__ {};                         // 协程配置
    std::any           ret__;                               // 协程返回值，会被传递给 config 中的 entry
    co_env*            env__ { nullptr };                   // 协程当前对应的运行环境
    std::atomic<int>   priority__ { CO_IDLE_CTX_PRIORITY }; // 优先级

    std::unordered_map<std::string, std::shared_ptr<inner_local_base>> locals__; // 协程局部存储

#ifdef __GNUC__
#ifdef __x86_64__

    co_byte* regs__[32] {};
#else
#error only supported x86_64
#endif
#endif

    co_ctx(co_stack* stack, const co_ctx_config& config);

public:
    void set_priority(int priority);
    int  priority() const;
    bool can_schedule() const;

    co_stack*            stack() const;
    co_state             state() const;
    co_byte**            regs();
    void                 set_state(co_state state);
    const co_ctx_config& config() const;
    std::any&            ret_ref();
    void                 set_env(co_env* env);
    co_env*              env() const;
    bool                 can_destroy() const;
    void                 lock_destroy();
    void                 unlock_destroy();
    void                 set_stack(co_stack* stack);
    bool                 can_move() const;
    std::string          name() const;
    co_id                id() const;

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
        locals__[name] = std::make_shared<inner_local<T>>();
    }
    return std::dynamic_pointer_cast<inner_local<T>>(locals__[name])->get();
}

CO_NAMESPACE_END