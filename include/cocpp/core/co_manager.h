_Pragma("once");

#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env_factory.h"
#include "cocpp/core/co_stack_factory.h"
#include "cocpp/sync/co_spinlock.h"
#include "cocpp/utils/co_any.h"
#include "cocpp/utils/co_noncopyable.h"
#include "cocpp/utils/co_singleton.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <mutex>
#include <set>

CO_NAMESPACE_BEGIN

class co_env;
class co_ctx;
class co_env_factory;
class co_ctx_factory;
class co_stack_factory;
class co_scheduler_factory;

class co_manager final : public co_singleton_static<co_manager>
{
    RegCoEvent(best_env_got, co_env*);
    RegCoEvent(env_created, co_env*);
    RegCoEvent(env_shared_stack_size_set, size_t);
    RegCoEvent(background_task_created);
    RegCoEvent(env_removed, co_env*);
    RegCoEvent(env_from_this_thread_created, co_env*);
    RegCoEvent(clean_up_set);
    RegCoEvent(env_routine_cleaned);
    RegCoEvent(base_thread_count_set, size_t);
    RegCoEvent(max_thread_count_set, size_t);
    RegCoEvent(ctx_redistributed);
    RegCoEvent(redundant_env_destroyed);
    RegCoEvent(timing_routine_finished);
    RegCoEvent(timing_duration_set);
    RegCoEvent(all_factory_destroyed);
    RegCoEvent(background_task_finished);
    RegCoEvent(ctx_created, co_ctx*);
    RegCoEvent(timing_routine_timout);

private:
    co_env_set env_set__ {
        .normal_env_count = 0,                                      // 正常环境数量
        .base_env_count   = std::thread::hardware_concurrency(),    // 基础环境数量
        .max_env_count    = std::thread::hardware_concurrency() * 2 // 最大环境数量
    };                                                              // 协程调度环境集合

    co_factory_set factory_set__ {
        .env_factory   = co_env_factory::instance(),  // 环境工厂
        .ctx_factory   = co_ctx_factory::instance(),  // 协程工厂
        .stack_factory = co_stack_factory::instance() // 堆栈工厂
    };                                                // 工厂集合

    bool                                         clean_up__ { false };                                // 是否需要清理协程调度环境
    std::mutex                                   clean_up_lock__;                                     // 清理协程调度环境锁
    std::list<std::future<void>>                 background_task__;                                   // 后台任务队列
    mutable std::mutex                           mu_timer_duration__;                                 // 定时器时间锁
    std::chrono::high_resolution_clock::duration timer_duration__ { std::chrono::milliseconds(10) };  // 定时器时间
    size_t                                       default_shared_stack_size__ = CO_DEFAULT_STACK_SIZE; // 默认共享堆栈大小
    std::function<bool()>                        need_free_mem_cb__ { [] { return false; } };         // 需要释放内存回调
    std::mutex                                   need_free_mem_cb_lock__;                             // 需要释放内存回调锁

    void    clean_env_routine__();              // 清理协程调度环境
    void    timer_routine__();                  // 定时器
    void    redistribute_ctx__();               // 重新分配协程
    void    force_schedule__();                 // 强制调度
    void    destroy_redundant_env__();          // 销毁冗余环境
    void    free_mem__();                       // 释放内存
    void    wait_background_task__();           // 等待后台任务
    void    set_clean_up__();                   // 设置清理协程调度环境
    void    destroy_all_factory__();            // 销毁所有工厂
    co_env* get_best_env__();                   // 获取最佳环境
    void    remove_env__(co_env* env);          // 移除环境
    void    subscribe_env_event__(co_env* env); // 订阅环境事件
    void    subscribe_ctx_event__(co_ctx* env); // 订阅协程事件
    void    subscribe_manager_event__();        // 订阅管理器事件
    void    create_background_task__();         // 创建后台任务
    void    create_env_from_this_thread__();    // 创建当前线程环境
    void    steal_ctx_routine__();              // 偷取协程

    co_manager(); // 构造函数
public:
    co_env* create_env(bool dont_auto_destory); // 创建环境
    co_ctx* create_and_schedule_ctx(
        const co_ctx_config&         config,
        std::function<void(co_any&)> entry,
        bool                         lock_destroy = true);                                    // 创建并调度协程
                                                                      //
    void    set_env_shared_stack_size(size_t size);                   // 设置环境共享堆栈大小
    co_env* current_env();                                            // 获取当前环境
    void    set_base_schedule_thread_count(size_t base_thread_count); // 设置基础调度线程数量
    void    set_max_schedule_thread_count(size_t max_thread_count);   // 设置最大调度线程数量
    void    set_if_free_mem_callback(std::function<bool()> cb);       // 设置是否释放内存回调
    void    set_timer_tick_duration(
           const std::chrono::high_resolution_clock::duration& duration);        // 设置定时器时间
    const std::chrono::high_resolution_clock::duration& timing_duration() const; // 获取定时器时间
    ~co_manager();                                                               // 析构函数

    CoMemberMethodProxy(factory_set__.ctx_factory, create_ctx);
    CoMemberMethodProxy(factory_set__.ctx_factory, destroy_ctx);
    CoMemberMethodProxy(factory_set__.stack_factory, create_stack);
    CoMemberMethodProxy(factory_set__.stack_factory, destroy_stack);

    friend class co_singleton_static<co_manager>;
};

CO_NAMESPACE_END