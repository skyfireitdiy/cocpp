_Pragma("once");

#include "cocpp/core/co_stack_factory.h"
#include "cocpp/mem/co_object_pool.h"
#include "cocpp/utils/co_singleton.h"

CO_NAMESPACE_BEGIN

class co_scheduler;
class co_stack_factory;
class co_scheduler_factory;
class co_env;

class co_env_factory final : public co_singleton<co_env_factory>
{
private:
    co_object_pool<co_env> env_pool__ { MAX_ENV_CACHE_COUNT };               // 运行环境对象池
    co_stack_factory*      stack_factory__ { co_stack_factory::instance() }; // 栈工厂
    co_ctx*                create_idle_ctx__();                              // 创建空闲协程

public:
    co_env* create_env(size_t stack_size);                  // 创建运行环境
    co_env* create_env_from_this_thread(size_t stack_size); // 从当前线程创建运行环境
    void    destroy_env(co_env* env);                       // 销毁运行环境
    void    free_obj_pool();                                // 释放对象池
};

CO_NAMESPACE_END