#pragma once

#include "co_ctx.h"
#include "co_nocopy.h"
#include "co_ret.h"
#include "co_stack.h"

#include <chrono>
#include <list>
#include <memory>
#include <optional>
#include <thread>

class co_manager;
class co_scheduler;

class co_env : public co_nocopy
{
public:
    virtual void                  init_ctx(co_ctx* ctx) = 0; // 初始化ctx
    virtual co_stack*             shared_stack() const  = 0; // 获取共享栈
    virtual void                  add_ctx(co_ctx* ctx)  = 0; // 添加ctx进行调度
    virtual std::optional<co_ret> wait_ctx(
        co_ctx*                         ctx,
        const std::chrono::nanoseconds& timeout)
        = 0;                                                         // 等待一个ctx在指定时间内执行完成，如果在指定时间内执行结束那么optional对象会被赋值，可以通过对co_ret类型进行强制类型转换获取到返回值，如果超时，返回空的optional对象
    virtual co_ret             wait_ctx(co_ctx* ctx)            = 0; // 等待，但是不会超时，所以肯定会有返回值
    virtual int                workload() const                 = 0; // 返回当前env的负载，为协程迁移和env数量动态管理提供参考
    virtual bool               has_scheduler_thread() const     = 0; // 是否有调度线程
    virtual co_env_state       state() const                    = 0; // 获取执行环境状态，参照 co_env_state 类型
    virtual void               set_state(co_env_state state)    = 0; // 设置执行环境状态
    virtual void               schedule_switch()                = 0; // 调度接口。此接口为关键接口，需要完成选择下一个调度协程，保存当前的执行环境，切换到下一个执行环境
    virtual void               remove_ctx(co_ctx* ctx)          = 0; // 删除一个ctx
    virtual co_ctx*            current_ctx() const              = 0; // 当前执行的ctx
    virtual co_ctx*            idle_ctx() const                 = 0; // 空闲ctx
    virtual void               stop_schedule()                  = 0; // 停止调度，在调用此函数后，此env失去调度ctx的能力
    virtual void               start_schedule()                 = 0; // 开始调度，调用此函数后，env具备调度ctx的能力
    virtual void               schedule_in_this_thread()        = 0; // 将此线程变成一个调度线程，注意，此函数在内部调用stop_schedule之前都不应该返回
    virtual void               set_manager(co_manager* manager) = 0; // 设置manager，将env与对应的manager绑定
    virtual co_manager*        manager() const                  = 0; // 获取manager
    virtual co_scheduler*      scheduler() const                = 0; // 获取调度器
    virtual bool               scheduled() const                = 0; // 获取是否调度过
    virtual void               reset_scheduled()                = 0; // 重试调度过的标记
    virtual void               lock_schedule()                  = 0; // 锁定调度，在此期间不发生调度
    virtual void               unlock_schedule()                = 0; // 解锁调度
    virtual std::list<co_ctx*> moveable_ctx_list()              = 0; // 获取当前可移动的ctx列表
    virtual void               take_ctx(co_ctx* ctx)            = 0; // 去除一个ctx，注意：与remove_ctx的区别是，此函数只是将ctx与env解绑，不会销毁ctx
    virtual bool               can_auto_destroy() const         = 0; // 当线程数量超过设定的最大数量的时候，是否允许被选中销毁
    virtual void               wake_up()                        = 0; // 唤醒当前env，当env没有ctx调度的时候就会休眠，使用此接口唤醒

    virtual ~co_env() = default;
};

extern thread_local co_env* current_env__;