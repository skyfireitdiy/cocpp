_Pragma("once");
#include "co_define.h"
#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/core/co_local.h"
#include "cocpp/core/co_type.h"
#include "cocpp/mem/co_object_pool.h"
#include "cocpp/utils/co_any.h"
#include "cocpp/utils/co_flag_manager.h"
#include "cocpp/utils/co_state_manager.h"

#include <exception>
#include <memory>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_env;
class co_stack;

class co_ctx final : private co_noncopyable
{
    RegCoEvent(finished);
    RegCoEvent(priority_changed, int, int);                            // 原优先级
    RegCoEvent(state_changed, co_state, co_state);                     // 原状态
    RegCoEvent(env_set, co_env*);                                      // 设置的env
    RegCoEvent(locked_destroy);                                        // 锁定销毁
    RegCoEvent(unlocked_destroy);                                      // 解锁销毁
    RegCoEvent(stack_set, co_stack*);                                  // 设置的stack
    RegCoEvent(state_set, co_state);                                   // 设置状态
    RegCoEvent(flag_set, size_t);                                      // 设置标识
    RegCoEvent(flag_reset, size_t);                                    // 重置标识
    RegCoEvent(wait_resource_state_entered, co_waited_rc_type, void*); // 等待资源状态进入
    RegCoEvent(wait_resource_state_leaved);                            // 等待资源状态离开

private:                                                                                                     //
    co_flag_manager<CO_CTX_FLAG_MAX>                                    flag_manager__;                      // flag 管理
    co_state_manager<co_state, co_state::suspended, co_state::finished> state_manager__;                     // 状态管理
    co_ctx_wait_data                                                    wait_data__ {};                      // 等待数据
    co_stack*                                                           stack__ { nullptr };                 // 当前栈空间
    co_ctx_config                                                       config__ {};                         // 协程配置
    co_any                                                              ret__;                               // 协程返回值
    co_env*                                                             env__ { nullptr };                   // 协程当前对应的运行环境
    mutable std::recursive_mutex                                        env_lock__;                          // 运行环境锁
    int                                                                 priority__ { CO_IDLE_CTX_PRIORITY }; // 优先级
    mutable std::recursive_mutex                                        priority_lock__;                     // 优先级锁
    std::map<std::string, std::shared_ptr<co_local_base>>               locals__;                            // 协程局部存储
    std::function<void(co_any&)>                                        entry__;                             // 协程入口函数
    std::exception_ptr                                                  exception__;                         // 异常
#ifdef __GNUC__                                                                                              //
#ifdef __x86_64__                                                                                            //
    co_byte* regs__[32] {};                                                                                  // 协程寄存器
#else                                                                                                        //
#error only supported x86_64
#endif
#endif
    co_ctx(co_stack* stack, const co_ctx_config& config, std::function<void(co_any&)> entry); // 构造函数

public:
    void                         set_priority(int priority);                                     // 设置优先级
    size_t                       priority() const;                                               // 获取优先级
    bool                         can_schedule() const;                                           // 判断是否可以调度
    co_stack*                    stack() const;                                                  // 获取当前栈空间
    co_byte**                    regs();                                                         // 获取寄存器
    const co_ctx_config&         config() const;                                                 // 获取配置
    co_any&                      ret_ref();                                                      // 获取返回值
    void                         set_env(co_env* env);                                           // 设置运行环境
    co_env*                      env() const;                                                    // 获取运行环境
    bool                         can_destroy() const;                                            // 判断是否可以销毁
    void                         lock_destroy();                                                 // 锁定销毁
    void                         unlock_destroy();                                               // 解锁销毁
    void                         set_stack(co_stack* stack);                                     // 设置栈空间
    bool                         can_move() const;                                               // 判断是否可以移动
    std::string                  name() const;                                                   // 获取名称
    co_id                        id() const;                                                     // 获取id
    void                         enter_wait_resource_state(co_waited_rc_type rc_type, void* rc); // 进入等待资源状态
    void                         leave_wait_resource_state();                                    // 离开等待资源状态
    std::function<void(co_any&)> entry() const;                                                  // 获取入口函数
    static void                  real_entry(co_ctx* ctx);                                        // 协程入口函数（内部实际的入口）
    void                         set_state(const co_state& state);                               // 设置ctx状态
    void                         set_flag(size_t flag);                                          // 设置标识
    void                         reset_flag(size_t flag);                                        // 重置标识
    void                         check_and_rethrow_exception();                                  // 检测并重新抛出异常

    template <typename T>
    T& local_storage(const std::string& name); // 获取局部存储

    CoConstMemberMethodProxy(&flag_manager__, test_flag);   // 测试标志
    CoConstMemberMethodProxy(&state_manager__, state);      // 获取状态
    CoConstMemberMethodProxy(&state_manager__, state_lock); // 获取状态锁

    friend class co_object_pool<co_ctx>;
};

// 模板实现

template <typename T>
T& co_ctx::local_storage(const std::string& name)
{
    if (!locals__.contains(name))
    {
        locals__[name] = std::make_shared<co_local<T>>();
    }
    return std::dynamic_pointer_cast<co_local<T>>(locals__[name])->get();
}

CO_NAMESPACE_END