#pragma once
#include "co_ctx_config.h"
#include "co_nocopy.h"

class co_env;
class co_ctx;
class co_env_factory;
class co_ctx_factory;
class co_stack_factory;
class co_scheduler_factory;

class co_manager : public co_nocopy
{
public:
    virtual co_env*               get_best_env()                                           = 0; // 挑选负载最低的env，如果没有可用的env，此接口还应该具备创建env的能力
    virtual void                  remove_env(co_env* env)                                  = 0; // 删除env，调用此接口后，env将会被回收（可能是异步回收）
    virtual void                  set_env_shared_stack_size(size_t size)                   = 0; // 设置env共享栈大小，创建env的时候会使用此大小为env创建共享栈
    virtual co_env_factory*       env_factory()                                            = 0; // 获取env工厂
    virtual co_ctx_factory*       ctx_factory()                                            = 0; // 获取ctx工厂
    virtual co_stack_factory*     stack_factory()                                          = 0; // 获取stack工厂
    virtual co_scheduler_factory* scheduler_factory()                                      = 0; // 获取scheduler工厂
    virtual void                  create_env_from_this_thread()                            = 0; // 从当前线程创建env
    virtual co_env*               current_env()                                            = 0; // 清理manager，此接口应该将所有的env销毁，并失去所有协程调度与管理能力，用于程序退出的时候清理资源
    virtual void                  set_clean_up()                                           = 0; // 设置清理标记，设置后，任何协程相关的api都不能再使用了
    virtual bool                  clean_up() const                                         = 0; // 是否已被清理
    virtual void                  set_base_schedule_thread_count(size_t base_thread_count) = 0; // 设置基础调度线程数量（如果在此数量之下，新建协程的时候会创建新的调度线程执行）
    virtual void                  set_max_schedule_thread_count(size_t max_thread_count)   = 0; // 设置最大调度线程数量（调度线程一旦超过此数量，如果调度线程变为空闲，就会被回收）
                                                                                                //
    virtual void set_timing_duration(
        const std::chrono::high_resolution_clock::duration& duration)
        = 0;                                                                                 // 设置定时任务时间间隔
    virtual const std::chrono::high_resolution_clock::duration& timing_duration() const = 0; // 获取定时任务时间间隔

    virtual ~co_manager() = default;
};
