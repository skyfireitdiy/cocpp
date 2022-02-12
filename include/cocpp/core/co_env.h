_Pragma("once");

#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_return_value.h"
#include "cocpp/core/co_sleep_controller.h"
#include "cocpp/core/co_stack_factory.h"
#include "cocpp/core/co_type.h"
#include "cocpp/mem/co_object_pool.h"
#include "cocpp/sync/co_spinlock.h"
#include "cocpp/utils/co_flag_manager.h"
#include "cocpp/utils/co_noncopyable.h"
#include "cocpp/utils/co_state_manager.h"

#include <atomic>
#include <bitset>
#include <chrono>
#include <condition_variable>
#include <future>
#include <list>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <thread>

CO_NAMESPACE_BEGIN

class co_ctx;

class co_env final : private co_noncopyable
{
    RegCoEvent(task_finished);
    RegCoEvent(ctx_added, co_ctx*);                                        // 被加入的ctx
    RegCoEvent(ctx_received, co_ctx*);                                     // 接收到ctx
    RegCoEvent(wait_ctx_timeout, co_ctx*);                                 // 等待的ctx
    RegCoEvent(wait_ctx_finished, co_ctx*);                                // 等待的ctx
    RegCoEvent(state_changed, co_env_state, co_env_state);                 // 原状态，当期状态
    RegCoEvent(switched_to, co_ctx*);                                      // 切换到
    RegCoEvent(ctx_removed, co_ctx*);                                      // 删除的ctx
    RegCoEvent(schedule_stopped);                                          // 停止调度
    RegCoEvent(schedule_started);                                          // 开始调度
    RegCoEvent(idle_waited);                                               // 空闲等待
    RegCoEvent(idle_waked);                                                // 空闲唤醒
    RegCoEvent(all_ctx_removed);                                           // 全部ctx被删除
    RegCoEvent(scheduled_flag_reset);                                      // 调度标志重置
    RegCoEvent(schedule_locked);                                           // 调度锁定
    RegCoEvent(schedule_unlocked);                                         // 调度解锁
    RegCoEvent(ctx_taken, co_ctx*);                                        // 协程被拿走
    RegCoEvent(ctx_initted, co_ctx*);                                      // 协程被初始化
    RegCoEvent(shared_stack_saved, co_ctx*);                               // 共享栈被保存
    RegCoEvent(shared_stack_restored, co_ctx*);                            // 共享栈被恢复
    RegCoEvent(all_moveable_ctx_taken, std::list<co_ctx*>);                // 全部可移动ctx被拿走
    RegCoEvent(one_moveable_ctx_taken, co_ctx*);                           // 一个可移动ctx被拿走
    RegCoEvent(this_thread_converted_to_schedule_thread, std::thread::id); // 当前线程转换为调度线程

private:                                                                                                                  //
    co_flag_manager<CO_ENV_FLAG_MAX>                                                flag_manager__;                       // 标志管理器
    co_state_manager<co_env_state, co_env_state::created, co_env_state::destorying> state_manager__;                      // 状态管理器
    std::future<void>                                                               worker__;                             // 工作线程
    co_sleep_controller                                                             sleep_controller__;                   // 睡眠控制器
    co_stack*                                                                       shared_stack__ { nullptr };           // 共享栈
    size_t                                                                          shared_stack_size__ { 0 };            // 共享栈大小
    std::once_flag                                                                  shared_stack_once_flag__;             // 共享栈初始化标志
    co_ctx* const                                                                   idle_ctx__ { nullptr };               // 空闲协程
    co_tid                                                                          schedule_thread_tid__ {};             // 调度线程tid
    std::vector<std::list<co_ctx*>>                                                 all_normal_ctx__ { CO_MAX_PRIORITY }; // 所有普通协程
    size_t                                                                          ctx_count__ { 0 };                    // 协程数量
    mutable std::recursive_mutex                                                    mu_normal_ctx__;                      // 普通协程锁
    co_ctx*                                                                         curr_ctx__ { nullptr };               // 当前协程
    mutable std::recursive_mutex                                                    mu_curr_ctx__;                        // 当前协程锁
    int                                                                             min_priority__ = 0;                   // 最小优先级
    mutable std::recursive_mutex                                                    mu_min_priority__;                    // 最小优先级锁
    std::recursive_mutex                                                            schedule_lock__;                      // 调度锁

    co_env(size_t shared_stack_size, co_ctx* idle_ctx, bool create_new_thread); // 构造函数
    void               start_schedule_routine__();                              // 启动调度线程
    void               remove_detached_ctx__();                                 // 删除已经detach的ctx
    void               remove_all_ctx__();                                      // 删除所有ctx
    co_ctx*            next_ctx__();                                            // 下一个ctx
    void               save_shared_stack__(co_ctx* ctx);                        // 保存共享栈
    void               restore_shared_stack__(co_ctx* ctx);                     // 恢复共享栈
    void               switch_shared_stack_ctx__();                             // 切换共享栈上的ctx
    void               switch_normal_ctx__();                                   // 切换普通ctx
    bool               need_sleep__();                                          // 是否需要睡眠
    co_ctx*            choose_ctx_from_normal_list__();                         // 从普通协程列表中选择ctx
    std::list<co_ctx*> all_ctx__();                                             // 所有ctx
    std::list<co_ctx*> all_scheduleable_ctx__() const;                          // 所有可调度ctx
    bool               can_schedule__() const;                                  // 是否可以调度
    void               update_min_priority__(int priority);                     // 更新最小优先级
    void               create_shared_stack__();                                 // 创建共享栈
    static size_t      get_valid_stack_size__(co_ctx* ctx);                     // 获取有效栈大小
    static void        update_ctx_state__(co_ctx* curr, co_ctx* next);          // 更新ctx状态
    struct
    {                                  //
        co_ctx* from { nullptr };      // 从哪个ctx切换
        co_ctx* to { nullptr };        // 切换到哪个ctx
        bool    need_switch { false }; // 是否需要切换
    } shared_stack_switch_info__;      // 共享栈上的ctx切换信息

public:
    void                           add_ctx(co_ctx* ctx);                                           // 添加ctx
    void                           move_ctx_to_here(co_ctx* ctx);                                  // 将ctx移动到这里
    std::optional<co_return_value> wait_ctx(co_ctx* ctx, const std::chrono::nanoseconds& timeout); // 等待ctx
    co_return_value                wait_ctx(co_ctx* ctx);                                          // 等待ctx
    size_t                         workload() const;                                               // 工作量
    void                           schedule_switch();                                              // 调度切换
    void                           remove_ctx(co_ctx* ctx);                                        // 删除ctx
    co_ctx*                        current_ctx() const;                                            // 当前ctx
    void                           stop_schedule();                                                // 停止调度
    void                           start_schedule();                                               // 启动调度
    void                           schedule_in_this_thread();                                      // 在这个线程调度
    void                           reset_scheduled_flag();                                         // 重置调度标志
    bool                           can_auto_destroy() const;                                       // 是否可以自动销毁
    co_tid                         schedule_thread_tid() const;                                    // 调度线程tid
    bool                           can_schedule_ctx() const;                                       // 是否可以调度ctx
    bool                           is_blocked() const;                                             // 是否被阻塞
    bool                           prepare_to_switch(co_ctx*& from, co_ctx*& to);                  // 准备切换
    void                           handle_priority_changed(int old, co_ctx* ctx);                  // 改变优先级
    void                           ctx_leave_wait_state(co_ctx* ctx);                              // ctx离开等待状态
    void                           ctx_enter_wait_state(co_ctx* ctx);                              // ctx进入等待状态
    std::list<co_ctx*>             take_all_movable_ctx();                                         // 拿走所有可移动的ctx
    co_ctx*                        take_one_movable_ctx();                                         // 拿走一个可移动的ctx
    void                           lock_schedule();                                                // 锁定调度
    void                           unlock_schedule();                                              // 解锁调度
    void                           set_state(const co_env_state& state);                           //  设置状态，覆盖父类函数
    void                           set_exclusive();                                                // 设置为独占模式
    bool                           exclusive() const;                                              // 是否为独占模式
    bool                           has_ctx() const;                                                // 是否有ctx
    size_t                         ctx_count() const;                                              // ctx数量

    CoConstMemberMethodProxy(&state_manager__, state);       // 获取状态
    CoConstMemberMethodProxy(&flag_manager__, test_flag);    // 测试标志
    CoMemberMethodProxy(&flag_manager__, set_flag);          // 设置标志
    CoMemberMethodProxy(&flag_manager__, reset_flag);        // 重置标志
    CoMemberMethodProxy(&sleep_controller__, sleep_if_need); // 如果需要睡眠，则睡眠
    CoMemberMethodProxy(&sleep_controller__, wake_up);       // 唤醒
    CoMemberMethodProxy(&sleep_controller__, sleep_lock);    // 睡眠锁

    friend class co_object_pool<co_env>;
    friend class co_env_factory;
};

extern thread_local co_env* current_env__;

CO_NAMESPACE_END