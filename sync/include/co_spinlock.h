_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_ctx;

class co_spinlock final : private co_noncopyable
{
private:
    std::atomic<co_ctx*> owner__;

public:
    void lock(co_ctx* const ctx);
    void unlock(co_ctx* const ctx);
};

CO_NAMESPACE_END