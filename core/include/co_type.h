_Pragma("once");

#include "co_define.h"
#include "co_spinlock.h"

#include <condition_variable>
#include <mutex>
#include <unordered_set>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_env;
class co_scheduler_factory;
class co_env_factory;
class co_ctx_factory;
class co_stack_factory;

using co_byte          = unsigned char;
using co_id            = unsigned long long;
using co_event_handler = unsigned long long;
using co_tid           = unsigned long long;

enum class co_state : unsigned char
{
    suspended, // 暂停
    running,   // 运行
    finished,  // 结束
};

enum class co_env_state : unsigned char
{
    idle,       // 空闲状态，此时可以将ctx加入到env中，或者释放此env
    busy,       // 繁忙状态，可以将ctx加入到env
    blocked,    // 阻塞状态，需要创建更多的env分担任务
    destorying, // 正在销毁状态，不再调度ctx
    created,    // 创建完成状态，此env不能用于调度ctx，通常是普通线程适配产生的env
};

struct co_ctx_wait_data
{
    co_spinlock mu { co_spinlock::lock_type::in_thread }; // 互斥锁
    int         type;                                     // 等待类型
    void*       resource;                                 // 等待资源
};

struct co_env_set
{
    std::unordered_set<co_env*> normal_set;                                                // 普通线程环境
    std::recursive_mutex        normal_lock;                                               // 普通线程环境互斥锁
    std::unordered_set<co_env*> expired_set;                                               // 过期线程环境
    std::recursive_mutex        expired_lock;                                              // 过期线程环境互斥锁
    std::condition_variable_any cond_expired_env;                                          // 过期线程环境条件变量
    unsigned int                normal_env_count;                                          // 普通线程环境数量
    co_spinlock                 mu_normal_env_count { co_spinlock::lock_type::in_thread }; // 普通线程环境数量互斥锁
    unsigned int                base_env_count;                                            // 基础线程环境数量
    unsigned int                max_env_count;                                             // 最大线程环境数量
};

struct co_factory_set
{
    co_env_factory* const   env_factory;   // 环境工厂
    co_ctx_factory* const   ctx_factory;   // 上下文工厂
    co_stack_factory* const stack_factory; // 堆栈工厂
};

CO_NAMESPACE_END