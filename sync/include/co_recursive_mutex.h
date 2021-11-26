_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"

CO_NAMESPACE_BEGIN

class co_ctx;

class co_recursive_mutex final : private co_noncopyable
{
private:
    co_spinlock          spinlock__;
    std::atomic<co_ctx*> owner__ { nullptr }; // 当前mutex的所有者
    std::list<co_ctx*>   waited_ctx_list__;   // 当前mutex上的等待队列
    unsigned long long   lock_count__ { 0 };  // 锁定次数
public:
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END