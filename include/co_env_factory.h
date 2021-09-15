#pragma once

class co_scheduler;
class co_stack_factory;
class co_env;
class co_ctx_factory;

// 协程运行环境工厂
class co_env_factory
{
public:
    virtual co_env* create_env(size_t stack_size)                      = 0; // 创建协程执行环境，需要创建一个新的调度线程与env绑定，传入的参数是执行环境开辟的用于共享栈协程的栈空间
    virtual co_env* create_env_from_this_thread(size_t stack_size)     = 0; // 从当前线程创建协程执行环境，将新创建的env与当前线程绑定
    virtual void    destroy_env(co_env* env)                           = 0; // 销毁执行环境，包括上述两个接口创建的env对象
    virtual void    set_stack_factory(co_stack_factory* stack_factory) = 0; // 设置stack工厂，用来创建共享栈
    virtual void    set_ctx_factory(co_ctx_factory* ctx_factory)       = 0; // 设置ctx工厂，用来创建空闲协程上下文，每个env都有一个idle协程，当所有的协程都被执行结束时，应该调度此协程
    virtual ~co_env_factory() {}
};