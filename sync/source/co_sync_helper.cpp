#include "co_sync_helper.h"
#include "co_ctx.h"
#include "co_spinlock.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void lock_yield__(co_ctx* ctx, co_spinlock& lk, std::function<bool()> checker)
{
    do
    {
        lk.unlock(ctx);
        this_co::yield();
        lk.lock(ctx);
    } while (checker());
}

CO_NAMESPACE_END