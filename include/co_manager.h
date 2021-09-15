#pragma once
#include "co_ctx_config.h"

class co_env;
class co_ctx;
class co_env_factory;
class co_ctx_factory;
class co_stack_factory;

class co_manager
{
public:
    virtual co_env*           get_best_env()                                   = 0; // 挑选负载最低的env，如果没有可用的env，此接口还应该具备创建env的能力
    virtual void              remove_env(co_env* env)                          = 0; // 删除env，调用此接口后，env将会被回收（可能是异步回收）
    virtual void              set_env_shared_stack_size(size_t size)           = 0; // 设置env共享栈大小，创建env的时候会使用此大小为env创建共享栈
    virtual void              set_env_factory(co_env_factory* env_factory)     = 0; // 设置env工厂
    virtual void              set_ctx_factory(co_ctx_factory* env_factory)     = 0; // 设置ctx工厂
    virtual void              set_stack_factory(co_stack_factory* env_factory) = 0; // 设置stack工厂
    virtual co_env_factory*   env_factory()                                    = 0; // 获取env工厂
    virtual co_ctx_factory*   ctx_factory()                                    = 0; // 获取ctx工厂
    virtual co_stack_factory* stack_factory()                                  = 0; // 获取stack工厂
    virtual void              create_env_from_this_thread()                    = 0; // 从当前线程创建env
    virtual co_env*           current_env()                                    = 0; // 清理manager，此接口应该将所有的env销毁，并失去所有协程调度与管理能力，用于程序退出的时候清理资源
    virtual void              clean_up()                                       = 0;
    virtual ~co_manager() {}
};
