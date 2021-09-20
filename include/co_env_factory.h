#pragma once

class co_manager;
class co_env;

// 协程运行环境工厂
class co_env_factory
{
public:
    virtual co_env* create_env(size_t stack_size)                  = 0; // 创建协程执行环境，需要创建一个新的调度线程与env绑定，传入的参数是执行环境开辟的用于共享栈协程的栈空间
    virtual co_env* create_env_from_this_thread(size_t stack_size) = 0; // 从当前线程创建协程执行环境，将新创建的env与当前线程绑定
    virtual void    destroy_env(co_env* env)                       = 0; // 销毁执行环境，包括上述两个接口创建的env对象
    virtual void    set_manager(co_manager* manager)               = 0; // 设置manager
    virtual ~co_env_factory() {}
};