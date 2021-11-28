#include "co_spinlock.h"
#include "co.h"
#include "co_define.h"
#include "co_error.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    auto    ctx  = co::current_ctx();
    co_ctx* null = nullptr;
    while (!owner__.compare_exchange_strong(null, ctx))
    {
        co::current_env()->schedule_switch(true);
        null = nullptr;
    }
    // CO_O_DEBUG("%p locked", ctx);
}

void co_spinlock::unlock()
{
    auto    ctx  = co::current_ctx();
    co_ctx* curr = ctx;
    // CO_O_DEBUG("%p unlock, owner is %p", ctx, owner__.load());
    if (!owner__.compare_exchange_strong(curr, nullptr))
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, curr);
        throw co_error("ctx is not owner[", ctx, "]");
    }
}

bool co_spinlock::try_lock()
{
    auto    ctx  = co::current_ctx();
    co_ctx* null = nullptr;
    if (owner__.compare_exchange_strong(null, ctx))
    {
        // CO_O_DEBUG("%p try locked", ctx);
        return true;
    }
    else
    {
        // CO_O_DEBUG("%p try lock failed", ctx);
        return false;
    }
}

CO_NAMESPACE_END