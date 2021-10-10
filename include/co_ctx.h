#pragma once

#include "co_ctx_config.h"
#include "co_flag_manager.h"
#include "co_nocopy.h"
#include "co_stack.h"
#include "co_type.h"

class co_env;

// 协程上下文
class co_ctx : public co_nocopy,
               public co_flag_manager<CO_CTX_FLAG_MAX>
{
public:
    virtual co_stack*            stack() const              = 0; // 获取协程栈
    virtual co_state             state() const              = 0; // 获取协程状态
    virtual co_byte**            regs()                     = 0; // 获取保存寄存器的缓冲区
    virtual void                 set_state(co_state state)  = 0; // 设置协程状态
    virtual const co_ctx_config& config() const             = 0; // 获取当前协程配置
    virtual std::any&            ret_ref()                  = 0; // 获取返回值的引用对象，协程执行结束后，返回值将存储到此any对象中
    virtual void                 set_env(co_env* env)       = 0; // 设置协程运行环境，该接口用于将ctx与env绑定起来，在M:N调度环境下，env可能会发生改变，需要通过此接口同步绑定关系
    virtual co_env*              env() const                = 0; // 获取绑定的env
    virtual void                 set_priority(int priority) = 0; // 设置优先级
    virtual int                  priority() const           = 0; // 优先级
    virtual bool                 can_destroy() const        = 0; // 是否可以被安全销毁
    virtual void                 lock_destroy()             = 0; // 锁定，不允许被销毁
    virtual void                 unlock_destroy()           = 0; // 解锁,允许被销毁

    virtual ~co_ctx() = default;
};
