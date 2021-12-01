#include "co_spinlock.h"
#include "co_define.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void co_spinlock::lock(co_ctx* const ctx)
{
    co_ctx* null = nullptr;
    while (!owner__.compare_exchange_strong(null, ctx))
    {
        this_co::yield();
        null = nullptr;
    }
}

void co_spinlock::unlock(co_ctx* const ctx)
{
    co_ctx* curr = ctx;
    while (!owner__.compare_exchange_strong(curr, nullptr))
    {
        this_co::yield();
        curr = ctx;
    }
}

CO_NAMESPACE_END